#include "router.hpp"
#include <llarp/iwp.h>
#include <llarp/link.h>
#include <llarp/proto.h>
#include <llarp/router.h>
#include <llarp/link_message.hpp>

#include "buffer.hpp"
#include "encode.hpp"
#include "logger.hpp"
#include "net.hpp"
#include "str.hpp"

#include <fstream>

namespace llarp
{
  void
  router_iter_config(llarp_config_iterator *iter, const char *section,
                     const char *key, const char *val);
}  // namespace llarp

llarp_router::llarp_router() : ready(false), inbound_msg_handler(this)
{
  llarp_rc_clear(&rc);
}

llarp_router::~llarp_router()
{
  llarp_rc_free(&rc);
}

bool
llarp_router::HandleRecvLinkMessage(llarp_link_session *session,
                                    llarp_buffer_t buf)
{
  if(inbound_msg_handler.ProcessFrom(session, buf))
  {
    return inbound_msg_handler.FlushReplies();
  }
  else
    return false;
}

bool
llarp_router::ProcessLRCM(llarp::LR_CommitMessage msg)
{
  return false;
}

void
llarp_router::try_connect(fs::path rcfile)
{
  byte_t tmp[MAX_RC_SIZE];
  llarp_rc remote = {0};
  llarp_buffer_t buf;
  llarp::StackBuffer< decltype(tmp) >(buf, tmp);
  // open file
  {
    std::ifstream f(rcfile, std::ios::binary);
    if(f.is_open())
    {
      f.seekg(0, std::ios::end);
      size_t sz = f.tellg();
      f.seekg(0, std::ios::beg);
      if(sz <= buf.sz)
      {
        f.read((char *)buf.base, sz);
      }
      else
        llarp::Error(__FILE__, rcfile, " too large");
    }
    else
    {
      llarp::Error(__FILE__, "failed to open ", rcfile);
      return;
    }
  }
  if(llarp_rc_bdecode(&remote, &buf))
  {
    if(llarp_rc_verify_sig(&crypto, &remote))
    {
      llarp::Debug(__FILE__, "verified signature");
      if(!llarp_router_try_connect(this, &remote))
      {
        llarp::Warn(__FILE__, "session already made");
      }
    }
    else
      llarp::Error(__FILE__, "failed to verify signature of RC");
  }
  else
    llarp::Error(__FILE__, "failed to decode RC");

  llarp_rc_free(&remote);
}

bool
llarp_router::EnsureIdentity()
{
  return llarp_findOrCreateIdentity(&crypto, ident_keyfile.c_str(), identity);
}

void
llarp_router::AddLink(struct llarp_link *link)
{
  links.push_back(link);
  ready = true;
}

bool
llarp_router::Ready()
{
  return ready;
}

bool
llarp_router::SaveRC()
{
  llarp::Debug(__FILE__, "verify RC signature");
  if(!llarp_rc_verify_sig(&crypto, &rc))
  {
    llarp::Error(__FILE__, "RC has bad signature not saving");
    return false;
  }

  byte_t tmp[MAX_RC_SIZE];
  auto buf = llarp::StackBuffer< decltype(tmp) >(tmp);

  if(llarp_rc_bencode(&rc, &buf))
  {
    std::ofstream f(our_rc_file);
    if(f.is_open())
    {
      f.write((char *)buf.base, buf.cur - buf.base);
      llarp::Info(__FILE__, "our RC saved to ", our_rc_file.c_str());
      return true;
    }
  }
  llarp::Error(__FILE__, "did not save RC to ", our_rc_file.c_str());
  return false;
}

void
llarp_router::Close()
{
  for(auto &link : links)
  {
    link->stop_link(link);
    link->free_impl(link);
    delete link;
  }
  links.clear();
}

void
llarp_router::on_try_connect_result(llarp_link_establish_job *job)
{
  if(job->session)
  {
    llarp_rc *remote = job->session->get_remote_router(job->session);
    if(remote)
    {
      llarp_router *router = static_cast< llarp_router * >(job->user);
      llarp::pubkey pubkey;
      memcpy(&pubkey[0], remote->pubkey, 32);
      char tmp[68] = {0};
      const char *pubkeystr =
          llarp::HexEncode< decltype(pubkey), decltype(tmp) >(pubkey, tmp);
      llarp::Info(__FILE__, "session established with ", pubkeystr);
      auto itr = router->pendingMessages.find(pubkey);
      if(itr != router->pendingMessages.end())
      {
        // flush pending
        if(itr->second.size())
        {
          llarp::Info(__FILE__, pubkeystr, " flush ", itr->second.size(),
                      " pending messages");
        }
        for(auto &msg : itr->second)
        {
          auto buf = llarp::Buffer< decltype(msg) >(msg);
          job->session->sendto(job->session, buf);
        }
        router->pendingMessages.erase(itr);
      }
      return;
    }
  }
  llarp::Info(__FILE__, "session not established");
}

void
llarp_router::Run()
{
  // zero out router contact
  llarp::Zero(&rc, sizeof(llarp_rc));
  // fill our address list
  rc.addrs = llarp_ai_list_new();
  for(auto link : links)
  {
    llarp_ai addr;
    link->get_our_address(link, &addr);
    llarp_ai_list_pushback(rc.addrs, &addr);
  };
  // set public key
  llarp_rc_set_pubkey(&rc, pubkey());

  llarp_rc_sign(&crypto, identity, &rc);

  if(!SaveRC())
  {
    return;
  }

  char tmp[68] = {0};

  llarp::pubkey ourPubkey;
  memcpy(&ourPubkey[0], pubkey(), 32);

  const char *us =
      llarp::HexEncode< llarp::pubkey, decltype(tmp) >(ourPubkey, tmp);

  llarp::Debug(__FILE__, "our router has public key ", us);

  // start links
  for(auto link : links)
  {
    int result = link->start_link(link, logic);
    if(result == -1)
      llarp::Warn(__FILE__, "Link ", link->name(), " failed to start");
    else
      llarp::Debug(__FILE__, "Link ", link->name(), " started");
  }

  for(const auto &itr : connect)
  {
    llarp::Info(__FILE__, "connecting to node ", itr.first);
    try_connect(itr.second);
  }
}

bool
llarp_router::iter_try_connect(llarp_router_link_iter *iter,
                               llarp_router *router, llarp_link *link)
{
  if(!link)
    return true;

  llarp_link_establish_job *job = new llarp_link_establish_job;

  if(!job)
    return true;
  llarp_ai *ai = static_cast< llarp_ai * >(iter->user);
  llarp_ai_copy(&job->ai, ai);
  job->timeout = 5000;
  job->result  = &llarp_router::on_try_connect_result;
  // give router as user pointer
  job->user = router;
  link->try_establish(link, job);
  return true;
}

extern "C" {

struct llarp_router *
llarp_init_router(struct llarp_threadpool *tp, struct llarp_ev_loop *netloop,
                  struct llarp_logic *logic)
{
  llarp_router *router = new llarp_router();
  if(router)
  {
    router->netloop = netloop;
    router->tp      = tp;
    router->logic   = logic;
    llarp_crypto_libsodium_init(&router->crypto);
  }
  return router;
}

bool
llarp_configure_router(struct llarp_router *router, struct llarp_config *conf)
{
  llarp_config_iterator iter;
  iter.user  = router;
  iter.visit = llarp::router_iter_config;
  llarp_config_iter(conf, &iter);
  if(!router->Ready())
  {
    return false;
  }
  return router->EnsureIdentity();
}

void
llarp_run_router(struct llarp_router *router)
{
  router->Run();
}

bool
llarp_router_try_connect(struct llarp_router *router, struct llarp_rc *remote)
{
  // try first address only
  llarp_ai addr;
  if(llarp_ai_list_index(remote->addrs, 0, &addr))
  {
    llarp_router_iterate_links(router,
                               {&addr, &llarp_router::iter_try_connect});
    return true;
  }

  return false;
}

void
llarp_rc_clear(struct llarp_rc *rc)
{
  // zero out router contact
  llarp::Zero(rc, sizeof(llarp_rc));
}

bool
llarp_rc_addr_list_iter(struct llarp_ai_list_iter *iter, struct llarp_ai *ai)
{
  struct llarp_rc *rc = (llarp_rc *)iter->user;
  llarp_ai_list_pushback(rc->addrs, ai);
  return true;
}

void
llarp_rc_set_addrs(struct llarp_rc *rc, struct llarp_alloc *mem,
                   struct llarp_ai_list *addr)
{
  rc->addrs = llarp_ai_list_new();
  struct llarp_ai_list_iter ai_itr;
  ai_itr.user  = rc;
  ai_itr.visit = &llarp_rc_addr_list_iter;
  llarp_ai_list_iterate(addr, &ai_itr);
}

void
llarp_rc_set_pubkey(struct llarp_rc *rc, uint8_t *pubkey)
{
  // set public key
  memcpy(rc->pubkey, pubkey, 32);
}

bool
llarp_findOrCreateIdentity(llarp_crypto *crypto, const char *fpath,
                           byte_t *secretkey)
{
  fs::path path(fpath);
  std::error_code ec;
  if(!fs::exists(path, ec))
  {
    crypto->identity_keygen(secretkey);
    std::ofstream f(path, std::ios::binary);
    if(f.is_open())
    {
      f.write((char *)secretkey, sizeof(llarp_seckey_t));
    }
  }
  std::ifstream f(path, std::ios::binary);
  if(f.is_open())
  {
    f.read((char *)secretkey, sizeof(llarp_seckey_t));
    return true;
  }
  return false;
}

bool
llarp_rc_write(struct llarp_rc *rc, const char *fpath)
{
  fs::path our_rc_file(fpath);
  byte_t tmp[MAX_RC_SIZE];
  auto buf = llarp::StackBuffer< decltype(tmp) >(tmp);

  if(llarp_rc_bencode(rc, &buf))
  {
    std::ofstream f(our_rc_file, std::ios::binary);
    if(f.is_open())
    {
      f.write((char *)buf.base, buf.cur - buf.base);
      return true;
    }
  }
  return false;
}

void
llarp_rc_sign(llarp_crypto *crypto, const byte_t *seckey, struct llarp_rc *rc)
{
  byte_t buf[MAX_RC_SIZE];
  auto signbuf = llarp::StackBuffer< decltype(buf) >(buf);
  // zero out previous signature
  llarp::Zero(rc->signature, sizeof(rc->signature));
  // encode
  if(llarp_rc_bencode(rc, &signbuf))
  {
    // sign
    signbuf.sz = signbuf.cur - signbuf.base;
    crypto->sign(rc->signature, seckey, signbuf);
  }
}

void
llarp_stop_router(struct llarp_router *router)
{
  if(router)
    router->Close();
}

void
llarp_router_iterate_links(struct llarp_router *router,
                           struct llarp_router_link_iter i)
{
  for(auto link : router->links)
    if(!i.visit(&i, router, link))
      return;
}

void
llarp_free_router(struct llarp_router **router)
{
  if(*router)
  {
    delete *router;
  }
  *router = nullptr;
}
}

namespace llarp
{
  void
  router_iter_config(llarp_config_iterator *iter, const char *section,
                     const char *key, const char *val)
  {
    llarp_router *self = static_cast< llarp_router * >(iter->user);
    int af;
    uint16_t proto;
    if(StrEq(val, "eth"))
    {
      af    = AF_PACKET;
      proto = LLARP_ETH_PROTO;
    }
    else
    {
      af    = AF_INET6;
      proto = std::atoi(val);
    }

    struct llarp_link *link = nullptr;
    if(StrEq(section, "iwp-links"))
    {
      link = new llarp_link;
      llarp::Zero(link, sizeof(llarp_link));

      llarp_iwp_args args = {
          .crypto       = &self->crypto,
          .logic        = self->logic,
          .cryptoworker = self->tp,
          .router       = self,
          .keyfile      = self->transport_keyfile.c_str(),
      };
      iwp_link_init(link, args);
      if(llarp_link_initialized(link))
      {
        if(link->configure(link, self->netloop, key, af, proto))
        {
          llarp_ai ai;
          link->get_our_address(link, &ai);
          llarp::Addr addr = ai;
          self->AddLink(link);
          return;
        }
      }
      llarp::Error(__FILE__, "link ", key, " failed to configure");
    }
    else if(StrEq(section, "iwp-connect"))
    {
      self->connect[key] = val;
    }
    else if(StrEq(section, "router"))
    {
      if(StrEq(key, "contact-file"))
      {
        self->our_rc_file = val;
      }
      if(StrEq(key, "transport-privkey"))
      {
        self->transport_keyfile = val;
      }
      if(StrEq(key, "ident-privkey"))
      {
        self->ident_keyfile = val;
      }
    }
  }

}  // namespace llarp
