﻿using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Security;
using System.Reflection;
using System.Security.Cryptography.X509Certificates;
using System.Windows.Forms;

namespace network.worktips.worktipsnet.win32.ui
{
    public partial class dlgBootstrap : Form
    {
        public dlgBootstrap()
        {
            InitializeComponent();
            if (Program.platform == PlatformID.Win32NT)
                default_path = Environment.ExpandEnvironmentVariables("%APPDATA%\\.worktipsnet");
            else
                default_path = Environment.ExpandEnvironmentVariables("%HOME%/.worktipsnet");
            label2.Text = String.Format("This file is automatically saved as {0}{1}{2}.", default_path, Path.DirectorySeparatorChar, rcName);
        }

        private WebClient wc;
        private string default_path;
        private const string rcName = "bootstrap.signed";

        private void button1_Click(object sender, EventArgs e)
        {
            Directory.CreateDirectory(default_path);
            var build = ((AssemblyInformationalVersionAttribute)Assembly
  .GetAssembly(typeof(main_frame))
  .GetCustomAttributes(typeof(AssemblyInformationalVersionAttribute), false)[0])
  .InformationalVersion;
            // add something more unique, this is the IE 5.0 default string
            try
            {
                ServicePointManager.ServerCertificateValidationCallback += cert_check;
                ServicePointManager.SecurityProtocol = (SecurityProtocolType)48 | 0 | (SecurityProtocolType)192 | (SecurityProtocolType)768 | (SecurityProtocolType)3072;
                wc = new WebClient();
                wc.Headers.Add("User-Agent", string.Format("Mozilla/4.0 (compatible; MSIE 5.0; Windows NT 5.0); worktipsnet-win32-managed-ui/{0}", build));
                wc.DownloadFile(uriBox.Text, string.Format("{0}{1}{2}", default_path, Path.DirectorySeparatorChar, rcName));
                MessageBox.Show("WorktipsNET node bootstrapped", "WorktipsNET", MessageBoxButtons.OK, MessageBoxIcon.Information);
                DialogResult = DialogResult.OK;
            }
            catch (Exception ex)
            {
                string worktipsnetExeString;
                Process worktipsnet_bootstrap = new Process();

                if (Program.platform == PlatformID.Win32NT)
                    worktipsnetExeString = String.Format("{0}\\worktipsnet-bootstrap.exe", Directory.GetCurrentDirectory());
                else
                    worktipsnetExeString = String.Format("{0}/worktipsnet-bootstrap", Directory.GetCurrentDirectory());

                worktipsnet_bootstrap.StartInfo.UseShellExecute = false;
                worktipsnet_bootstrap.StartInfo.CreateNoWindow = true;
                worktipsnet_bootstrap.StartInfo.WorkingDirectory = Directory.GetCurrentDirectory();
                worktipsnet_bootstrap.StartInfo.FileName = worktipsnetExeString;
                worktipsnet_bootstrap.StartInfo.Arguments = string.Format("--cacert rootcerts.pem -L {0} --output \"{1}{2}{3}\"", uriBox.Text, default_path, Path.DirectorySeparatorChar, rcName);
                worktipsnet_bootstrap.Start();
                worktipsnet_bootstrap.WaitForExit();
                if (worktipsnet_bootstrap.ExitCode == 0)
                {
                    DialogResult = DialogResult.OK;
                    MessageBox.Show("WorktipsNET node bootstrapped", "WorktipsNET", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    MessageBox.Show(string.Format("An error occured while downloading data. {0}", ex.Message), "Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
                    DialogResult = DialogResult.Abort;
                }
            }
            Close();
        }

        private bool cert_check(object sender, X509Certificate cert, X509Chain chain, SslPolicyErrors error)
        { 
        // If the certificate is a valid, signed certificate, return true.
        if (error == System.Net.Security.SslPolicyErrors.None)
        {
            return true;
        }

        MessageBox.Show(string.Format("X509Certificate [{0}] Policy Error: '{1}'",
            cert.Subject,
            error.ToString()), "SSL Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);

        return false;
        }

        private void button1_Click_1(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }
    }
}
