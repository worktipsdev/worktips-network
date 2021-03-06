/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class network_worktips_worktipsnet_WorktipsnetVPN */

#ifndef _Included_network_worktips_worktipsnet_WorktipsnetVPN
#define _Included_network_worktips_worktipsnet_WorktipsnetVPN
#ifdef __cplusplus
extern "C"
{
#endif
  /*
   * Class:     network_worktips_worktipsnet_WorktipsnetVPN
   * Method:    PacketSize
   * Signature: ()I
   */
  JNIEXPORT jint JNICALL
  Java_network_worktips_worktipsnet_WorktipsnetVPN_PacketSize(JNIEnv *, jclass);

  /*
   * Class:     network_worktips_worktipsnet_WorktipsnetVPN
   * Method:    Alloc
   * Signature: ()Ljava/nio/Buffer;
   */
  JNIEXPORT jobject JNICALL
  Java_network_worktips_worktipsnet_WorktipsnetVPN_Alloc(JNIEnv *, jclass);

  /*
   * Class:     network_worktips_worktipsnet_WorktipsnetVPN
   * Method:    Free
   * Signature: (Ljava/nio/Buffer;)V
   */
  JNIEXPORT void JNICALL
  Java_network_worktips_worktipsnet_WorktipsnetVPN_Free(JNIEnv *, jclass, jobject);

  /*
   * Class:     network_worktips_worktipsnet_WorktipsnetVPN
   * Method:    Stop
   * Signature: ()V
   */
  JNIEXPORT void JNICALL
  Java_network_worktips_worktipsnet_WorktipsnetVPN_Stop(JNIEnv *, jobject);

  /*
   * Class:     network_worktips_worktipsnet_WorktipsnetVPN
   * Method:    ReadPkt
   * Signature: (Ljava/nio/ByteBuffer;)I
   */
  JNIEXPORT jint JNICALL
  Java_network_worktips_worktipsnet_WorktipsnetVPN_ReadPkt(JNIEnv *, jobject, jobject);

  /*
   * Class:     network_worktips_worktipsnet_WorktipsnetVPN
   * Method:    WritePkt
   * Signature: (Ljava/nio/ByteBuffer;)Z
   */
  JNIEXPORT jboolean JNICALL
  Java_network_worktips_worktipsnet_WorktipsnetVPN_WritePkt(JNIEnv *, jobject, jobject);

  /*
   * Class:     network_worktips_worktipsnet_WorktipsnetVPN
   * Method:    SetInfo
   * Signature: (Lnetwork/worktips/worktipsnet/WorktipsnetVPN/VPNInfo;)V
   */
  JNIEXPORT void JNICALL
  Java_network_worktips_worktipsnet_WorktipsnetVPN_SetInfo(JNIEnv *, jobject, jobject);
#ifdef __cplusplus
}
#endif
#endif
