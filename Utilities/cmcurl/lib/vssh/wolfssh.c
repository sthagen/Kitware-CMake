/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * SPDX-License-Identifier: curl
 *
 ***************************************************************************/

#include "../curl_setup.h"

#ifdef USE_WOLFSSH

#include <limits.h>

#include "../urldata.h"
#include "../url.h"
#include "../cfilters.h"
#include "../connect.h"
#include "../sendf.h"
#include "../progress.h"
#include "curl_path.h"
#include "../transfer.h"
#include "../speedcheck.h"
#include "../select.h"
#include "../multiif.h"
#include "../curlx/warnless.h"
#include "../strdup.h"

/* The last 3 #include files should be in this order */
#include "../curl_printf.h"
#include "../curl_memory.h"
#include "../memdebug.h"

static CURLcode wssh_connect(struct Curl_easy *data, bool *done);
static CURLcode wssh_multi_statemach(struct Curl_easy *data, bool *done);
static CURLcode wssh_do(struct Curl_easy *data, bool *done);
#if 0
static CURLcode wscp_done(struct Curl_easy *data,
                          CURLcode, bool premature);
static CURLcode wscp_doing(struct Curl_easy *data,
                           bool *dophase_done);
static CURLcode wscp_disconnect(struct Curl_easy *data,
                                struct connectdata *conn,
                                bool dead_connection);
#endif
static CURLcode wsftp_done(struct Curl_easy *data,
                           CURLcode, bool premature);
static CURLcode wsftp_doing(struct Curl_easy *data,
                            bool *dophase_done);
static CURLcode wsftp_disconnect(struct Curl_easy *data,
                                 struct connectdata *conn,
                                 bool dead);
static int wssh_getsock(struct Curl_easy *data,
                        struct connectdata *conn,
                        curl_socket_t *sock);
static CURLcode wssh_setup_connection(struct Curl_easy *data,
                                      struct connectdata *conn);
static void wssh_sshc_cleanup(struct ssh_conn *sshc);

#if 0
/*
 * SCP protocol handler.
 */

const struct Curl_handler Curl_handler_scp = {
  "SCP",                                /* scheme */
  wssh_setup_connection,                /* setup_connection */
  wssh_do,                              /* do_it */
  wscp_done,                            /* done */
  ZERO_NULL,                            /* do_more */
  wssh_connect,                         /* connect_it */
  wssh_multi_statemach,                 /* connecting */
  wscp_doing,                           /* doing */
  wssh_getsock,                         /* proto_getsock */
  wssh_getsock,                         /* doing_getsock */
  ZERO_NULL,                            /* domore_getsock */
  wssh_getsock,                         /* perform_getsock */
  wscp_disconnect,                      /* disconnect */
  ZERO_NULL,                            /* write_resp */
  ZERO_NULL,                            /* write_resp_hd */
  ZERO_NULL,                            /* connection_check */
  ZERO_NULL,                            /* attach connection */
  ZERO_NULL,                            /* follow */
  PORT_SSH,                             /* defport */
  CURLPROTO_SCP,                        /* protocol */
  PROTOPT_DIRLOCK | PROTOPT_CLOSEACTION
  | PROTOPT_NOURLQUERY                  /* flags */
};

#endif

/*
 * SFTP protocol handler.
 */

const struct Curl_handler Curl_handler_sftp = {
  "SFTP",                               /* scheme */
  wssh_setup_connection,                /* setup_connection */
  wssh_do,                              /* do_it */
  wsftp_done,                           /* done */
  ZERO_NULL,                            /* do_more */
  wssh_connect,                         /* connect_it */
  wssh_multi_statemach,                 /* connecting */
  wsftp_doing,                          /* doing */
  wssh_getsock,                         /* proto_getsock */
  wssh_getsock,                         /* doing_getsock */
  ZERO_NULL,                            /* domore_getsock */
  wssh_getsock,                         /* perform_getsock */
  wsftp_disconnect,                     /* disconnect */
  ZERO_NULL,                            /* write_resp */
  ZERO_NULL,                            /* write_resp_hd */
  ZERO_NULL,                            /* connection_check */
  ZERO_NULL,                            /* attach connection */
  ZERO_NULL,                            /* follow */
  PORT_SSH,                             /* defport */
  CURLPROTO_SFTP,                       /* protocol */
  CURLPROTO_SFTP,                       /* family */
  PROTOPT_DIRLOCK | PROTOPT_CLOSEACTION
  | PROTOPT_NOURLQUERY                  /* flags */
};

/*
 * SSH State machine related code
 */
/* This is the ONLY way to change SSH state! */
static void wssh_state(struct Curl_easy *data,
                       struct ssh_conn *sshc,
                       sshstate nowstate)
{
#if defined(DEBUGBUILD) && !defined(CURL_DISABLE_VERBOSE_STRINGS)
  /* for debug purposes */
  static const char * const names[] = {
    "SSH_STOP",
    "SSH_INIT",
    "SSH_S_STARTUP",
    "SSH_HOSTKEY",
    "SSH_AUTHLIST",
    "SSH_AUTH_PKEY_INIT",
    "SSH_AUTH_PKEY",
    "SSH_AUTH_PASS_INIT",
    "SSH_AUTH_PASS",
    "SSH_AUTH_AGENT_INIT",
    "SSH_AUTH_AGENT_LIST",
    "SSH_AUTH_AGENT",
    "SSH_AUTH_HOST_INIT",
    "SSH_AUTH_HOST",
    "SSH_AUTH_KEY_INIT",
    "SSH_AUTH_KEY",
    "SSH_AUTH_GSSAPI",
    "SSH_AUTH_DONE",
    "SSH_SFTP_INIT",
    "SSH_SFTP_REALPATH",
    "SSH_SFTP_QUOTE_INIT",
    "SSH_SFTP_POSTQUOTE_INIT",
    "SSH_SFTP_QUOTE",
    "SSH_SFTP_NEXT_QUOTE",
    "SSH_SFTP_QUOTE_STAT",
    "SSH_SFTP_QUOTE_SETSTAT",
    "SSH_SFTP_QUOTE_SYMLINK",
    "SSH_SFTP_QUOTE_MKDIR",
    "SSH_SFTP_QUOTE_RENAME",
    "SSH_SFTP_QUOTE_RMDIR",
    "SSH_SFTP_QUOTE_UNLINK",
    "SSH_SFTP_QUOTE_STATVFS",
    "SSH_SFTP_GETINFO",
    "SSH_SFTP_FILETIME",
    "SSH_SFTP_TRANS_INIT",
    "SSH_SFTP_UPLOAD_INIT",
    "SSH_SFTP_CREATE_DIRS_INIT",
    "SSH_SFTP_CREATE_DIRS",
    "SSH_SFTP_CREATE_DIRS_MKDIR",
    "SSH_SFTP_READDIR_INIT",
    "SSH_SFTP_READDIR",
    "SSH_SFTP_READDIR_LINK",
    "SSH_SFTP_READDIR_BOTTOM",
    "SSH_SFTP_READDIR_DONE",
    "SSH_SFTP_DOWNLOAD_INIT",
    "SSH_SFTP_DOWNLOAD_STAT",
    "SSH_SFTP_CLOSE",
    "SSH_SFTP_SHUTDOWN",
    "SSH_SCP_TRANS_INIT",
    "SSH_SCP_UPLOAD_INIT",
    "SSH_SCP_DOWNLOAD_INIT",
    "SSH_SCP_DOWNLOAD",
    "SSH_SCP_DONE",
    "SSH_SCP_SEND_EOF",
    "SSH_SCP_WAIT_EOF",
    "SSH_SCP_WAIT_CLOSE",
    "SSH_SCP_CHANNEL_FREE",
    "SSH_SESSION_DISCONNECT",
    "SSH_SESSION_FREE",
    "QUIT"
  };

  /* a precaution to make sure the lists are in sync */
  DEBUGASSERT(CURL_ARRAYSIZE(names) == SSH_LAST);

  if(sshc->state != nowstate) {
    infof(data, "wolfssh %p state change from %s to %s",
          (void *)sshc, names[sshc->state], names[nowstate]);
  }
#endif
  (void)data;
  sshc->state = nowstate;
}

static CURLcode wscp_send(struct Curl_easy *data, int sockindex,
                          const void *mem, size_t len, bool eos,
                          size_t *pnwritten)
{
  (void)data;
  (void)sockindex; /* we only support SCP on the fixed known primary socket */
  (void)mem;
  (void)len;
  (void)eos;
  *pnwritten = 0;
  return CURLE_OK;
}

static CURLcode wscp_recv(struct Curl_easy *data, int sockindex,
                          char *mem, size_t len, size_t *pnread)
{
  (void)data;
  (void)sockindex; /* we only support SCP on the fixed known primary socket */
  (void)mem;
  (void)len;
  *pnread = 0;

  return CURLE_OK;
}

/* return number of sent bytes */
static CURLcode wsftp_send(struct Curl_easy *data, int sockindex,
                           const void *mem, size_t len, bool eos,
                           size_t *pnwritten)
{
  struct connectdata *conn = data->conn;
  struct ssh_conn *sshc = Curl_conn_meta_get(conn, CURL_META_SSH_CONN);
  word32 offset[2];
  int rc;
  (void)sockindex;
  (void)eos;

  *pnwritten = 0;
  if(!sshc)
    return CURLE_FAILED_INIT;

  offset[0] = (word32)sshc->offset & 0xFFFFFFFF;
  offset[1] = (word32)(sshc->offset >> 32) & 0xFFFFFFFF;

  rc = wolfSSH_SFTP_SendWritePacket(sshc->ssh_session, sshc->handle,
                                    sshc->handleSz,
                                    &offset[0],
                                    (byte *)CURL_UNCONST(mem), (word32)len);

  if(rc == WS_FATAL_ERROR)
    rc = wolfSSH_get_error(sshc->ssh_session);
  if(rc == WS_WANT_READ) {
    conn->waitfor = KEEP_RECV;
    return CURLE_AGAIN;
  }
  else if(rc == WS_WANT_WRITE) {
    conn->waitfor = KEEP_SEND;
    return CURLE_AGAIN;
  }
  if(rc < 0) {
    failf(data, "wolfSSH_SFTP_SendWritePacket returned %d", rc);
    return CURLE_SEND_ERROR;
  }
  DEBUGASSERT(rc == (int)len);
  *pnwritten = (size_t)rc;
  sshc->offset += *pnwritten;
  infof(data, "sent %zu bytes SFTP from offset %" FMT_OFF_T,
        *pnwritten, sshc->offset);
  return CURLE_OK;
}

/*
 * Return number of received (decrypted) bytes
 * or <0 on error
 */
static CURLcode wsftp_recv(struct Curl_easy *data, int sockindex,
                           char *mem, size_t len, size_t *pnread)
{
  int rc;
  struct connectdata *conn = data->conn;
  struct ssh_conn *sshc = Curl_conn_meta_get(conn, CURL_META_SSH_CONN);
  word32 offset[2];
  (void)sockindex;

  *pnread = 0;
  if(!sshc)
    return CURLE_FAILED_INIT;

  offset[0] = (word32)sshc->offset & 0xFFFFFFFF;
  offset[1] = (word32)(sshc->offset >> 32) & 0xFFFFFFFF;

  rc = wolfSSH_SFTP_SendReadPacket(sshc->ssh_session, sshc->handle,
                                   sshc->handleSz,
                                   &offset[0],
                                   (byte *)mem, (word32)len);
  if(rc == WS_FATAL_ERROR)
    rc = wolfSSH_get_error(sshc->ssh_session);
  if(rc == WS_WANT_READ) {
    conn->waitfor = KEEP_RECV;
    return CURLE_AGAIN;
  }
  else if(rc == WS_WANT_WRITE) {
    conn->waitfor = KEEP_SEND;
    return CURLE_AGAIN;
  }

  DEBUGASSERT(rc <= (int)len);

  if(rc < 0) {
    failf(data, "wolfSSH_SFTP_SendReadPacket returned %d", rc);
    return CURLE_RECV_ERROR;
  }
  *pnread = (size_t)rc;
  sshc->offset += *pnread;
  return CURLE_OK;
}

static void wssh_easy_dtor(void *key, size_t klen, void *entry)
{
  struct SSHPROTO *sshp = entry;
  (void)key;
  (void)klen;
  Curl_safefree(sshp->path);
  free(sshp);
}

static void wssh_conn_dtor(void *key, size_t klen, void *entry)
{
  struct ssh_conn *sshc = entry;
  (void)key;
  (void)klen;
  wssh_sshc_cleanup(sshc);
  free(sshc);
}

/*
 * SSH setup and connection
 */
static CURLcode wssh_setup_connection(struct Curl_easy *data,
                                      struct connectdata *conn)
{
  struct ssh_conn *sshc;
  struct SSHPROTO *sshp;
  (void)conn;

  sshc = calloc(1, sizeof(*sshc));
  if(!sshc)
    return CURLE_OUT_OF_MEMORY;

  sshc->initialised = TRUE;
  if(Curl_conn_meta_set(conn, CURL_META_SSH_CONN, sshc, wssh_conn_dtor))
    return CURLE_OUT_OF_MEMORY;

  sshp = calloc(1, sizeof(*sshp));
  if(!sshp ||
     Curl_meta_set(data, CURL_META_SSH_EASY, sshp, wssh_easy_dtor))
    return CURLE_OUT_OF_MEMORY;

  return CURLE_OK;
}

static int userauth(byte authtype,
                    WS_UserAuthData* authdata,
                    void *ctx)
{
  struct Curl_easy *data = ctx;
  DEBUGF(infof(data, "wolfssh callback: type %s",
               authtype == WOLFSSH_USERAUTH_PASSWORD ? "PASSWORD" :
               "PUBLICCKEY"));
  if(authtype == WOLFSSH_USERAUTH_PASSWORD) {
    authdata->sf.password.password = (byte *)data->conn->passwd;
    authdata->sf.password.passwordSz = (word32) strlen(data->conn->passwd);
  }

  return 0;
}

static CURLcode wssh_connect(struct Curl_easy *data, bool *done)
{
  struct connectdata *conn = data->conn;
  struct ssh_conn *sshc = Curl_conn_meta_get(conn, CURL_META_SSH_CONN);
  struct SSHPROTO *sshp = Curl_meta_get(data, CURL_META_SSH_EASY);
  curl_socket_t sock = conn->sock[FIRSTSOCKET];
  int rc;

  if(!sshc || !sshp)
    return CURLE_FAILED_INIT;

  /* We default to persistent connections. We set this already in this connect
     function to make the reuse checks properly be able to check this bit. */
  connkeep(conn, "SSH default");

  if(conn->handler->protocol & CURLPROTO_SCP) {
    conn->recv[FIRSTSOCKET] = wscp_recv;
    conn->send[FIRSTSOCKET] = wscp_send;
  }
  else {
    conn->recv[FIRSTSOCKET] = wsftp_recv;
    conn->send[FIRSTSOCKET] = wsftp_send;
  }
  sshc->ctx = wolfSSH_CTX_new(WOLFSSH_ENDPOINT_CLIENT, NULL);
  if(!sshc->ctx) {
    failf(data, "No wolfSSH context");
    goto error;
  }

  sshc->ssh_session = wolfSSH_new(sshc->ctx);
  if(!sshc->ssh_session) {
    failf(data, "No wolfSSH session");
    goto error;
  }

  rc = wolfSSH_SetUsername(sshc->ssh_session, conn->user);
  if(rc != WS_SUCCESS) {
    failf(data, "wolfSSH failed to set username");
    goto error;
  }

  /* set callback for authentication */
  wolfSSH_SetUserAuth(sshc->ctx, userauth);
  wolfSSH_SetUserAuthCtx(sshc->ssh_session, data);

  rc = wolfSSH_set_fd(sshc->ssh_session, (int)sock);
  if(rc) {
    failf(data, "wolfSSH failed to set socket");
    goto error;
  }

#if 0
  wolfSSH_Debugging_ON();
#endif

  *done = TRUE;
  if(conn->handler->protocol & CURLPROTO_SCP)
    wssh_state(data, sshc, SSH_INIT);
  else
    wssh_state(data, sshc, SSH_SFTP_INIT);

  return wssh_multi_statemach(data, done);
error:
  wssh_sshc_cleanup(sshc);
  return CURLE_FAILED_INIT;
}

/*
 * wssh_statemach_act() runs the SSH state machine as far as it can without
 * blocking and without reaching the end. The data the pointer 'block' points
 * to will be set to TRUE if the wolfssh function returns EAGAIN meaning it
 * wants to be called again when the socket is ready
 */

static CURLcode wssh_statemach_act(struct Curl_easy *data,
                                   struct ssh_conn *sshc,
                                   bool *block)
{
  CURLcode result = CURLE_OK;
  struct connectdata *conn = data->conn;
  struct SSHPROTO *sftp_scp = Curl_meta_get(data, CURL_META_SSH_EASY);
  WS_SFTPNAME *name;
  int rc = 0;
  *block = FALSE; /* we are not blocking by default */

  if(!sftp_scp)
    return CURLE_FAILED_INIT;

  do {
    switch(sshc->state) {
    case SSH_INIT:
      wssh_state(data, sshc, SSH_S_STARTUP);
      break;

    case SSH_S_STARTUP:
      rc = wolfSSH_connect(sshc->ssh_session);
      if(rc != WS_SUCCESS)
        rc = wolfSSH_get_error(sshc->ssh_session);
      if(rc == WS_WANT_READ) {
        *block = TRUE;
        conn->waitfor = KEEP_RECV;
        return CURLE_OK;
      }
      else if(rc == WS_WANT_WRITE) {
        *block = TRUE;
        conn->waitfor = KEEP_SEND;
        return CURLE_OK;
      }
      else if(rc != WS_SUCCESS) {
        wssh_state(data, sshc, SSH_STOP);
        return CURLE_SSH;
      }
      infof(data, "wolfssh connected");
      wssh_state(data, sshc, SSH_STOP);
      break;
    case SSH_STOP:
      break;

    case SSH_SFTP_INIT:
      rc = wolfSSH_SFTP_connect(sshc->ssh_session);
      if(rc != WS_SUCCESS)
        rc = wolfSSH_get_error(sshc->ssh_session);
      if(rc == WS_WANT_READ) {
        *block = TRUE;
        conn->waitfor = KEEP_RECV;
        return CURLE_OK;
      }
      else if(rc == WS_WANT_WRITE) {
        *block = TRUE;
        conn->waitfor = KEEP_SEND;
        return CURLE_OK;
      }
      else if(rc == WS_SUCCESS) {
        infof(data, "wolfssh SFTP connected");
        wssh_state(data, sshc, SSH_SFTP_REALPATH);
      }
      else {
        failf(data, "wolfssh SFTP connect error %d", rc);
        return CURLE_SSH;
      }
      break;
    case SSH_SFTP_REALPATH:
      name = wolfSSH_SFTP_RealPath(sshc->ssh_session,
                                   (char *)CURL_UNCONST("."));
      rc = wolfSSH_get_error(sshc->ssh_session);
      if(rc == WS_WANT_READ) {
        *block = TRUE;
        conn->waitfor = KEEP_RECV;
        return CURLE_OK;
      }
      else if(rc == WS_WANT_WRITE) {
        *block = TRUE;
        conn->waitfor = KEEP_SEND;
        return CURLE_OK;
      }
      else if(name && (rc == WS_SUCCESS)) {
        sshc->homedir = Curl_memdup0(name->fName, name->fSz);
        if(!sshc->homedir)
          sshc->actualcode = CURLE_OUT_OF_MEMORY;
        wolfSSH_SFTPNAME_list_free(name);
        wssh_state(data, sshc, SSH_STOP);
        return CURLE_OK;
      }
      failf(data, "wolfssh SFTP realpath %d", rc);
      return CURLE_SSH;

    case SSH_SFTP_QUOTE_INIT:
      result = Curl_getworkingpath(data, sshc->homedir, &sftp_scp->path);
      if(result) {
        sshc->actualcode = result;
        wssh_state(data, sshc, SSH_STOP);
        break;
      }

      if(data->set.quote) {
        infof(data, "Sending quote commands");
        sshc->quote_item = data->set.quote;
        wssh_state(data, sshc, SSH_SFTP_QUOTE);
      }
      else {
        wssh_state(data, sshc, SSH_SFTP_GETINFO);
      }
      break;
    case SSH_SFTP_GETINFO:
      if(data->set.get_filetime) {
        wssh_state(data, sshc, SSH_SFTP_FILETIME);
      }
      else {
        wssh_state(data, sshc, SSH_SFTP_TRANS_INIT);
      }
      break;
    case SSH_SFTP_TRANS_INIT:
      if(data->state.upload)
        wssh_state(data, sshc, SSH_SFTP_UPLOAD_INIT);
      else {
        if(sftp_scp->path[strlen(sftp_scp->path)-1] == '/')
          wssh_state(data, sshc, SSH_SFTP_READDIR_INIT);
        else
          wssh_state(data, sshc, SSH_SFTP_DOWNLOAD_INIT);
      }
      break;
    case SSH_SFTP_UPLOAD_INIT: {
      word32 flags;
      WS_SFTP_FILEATRB createattrs;
      if(data->state.resume_from) {
        WS_SFTP_FILEATRB attrs;
        if(data->state.resume_from < 0) {
          rc = wolfSSH_SFTP_STAT(sshc->ssh_session, sftp_scp->path,
                                 &attrs);
          if(rc != WS_SUCCESS)
            break;

          if(rc) {
            data->state.resume_from = 0;
          }
          else {
            curl_off_t size = ((curl_off_t)attrs.sz[1] << 32) | attrs.sz[0];
            if(size < 0) {
              failf(data, "Bad file size (%" FMT_OFF_T ")", size);
              return CURLE_BAD_DOWNLOAD_RESUME;
            }
            data->state.resume_from = size;
          }
        }
      }

      if(data->set.remote_append)
        /* Try to open for append, but create if nonexisting */
        flags = WOLFSSH_FXF_WRITE|WOLFSSH_FXF_CREAT|WOLFSSH_FXF_APPEND;
      else if(data->state.resume_from > 0)
        /* If we have restart position then open for append */
        flags = WOLFSSH_FXF_WRITE|WOLFSSH_FXF_APPEND;
      else
        /* Clear file before writing (normal behavior) */
        flags = WOLFSSH_FXF_WRITE|WOLFSSH_FXF_CREAT|WOLFSSH_FXF_TRUNC;

      memset(&createattrs, 0, sizeof(createattrs));
      createattrs.per = (word32)data->set.new_file_perms;
      sshc->handleSz = sizeof(sshc->handle);
      rc = wolfSSH_SFTP_Open(sshc->ssh_session, sftp_scp->path,
                             flags, &createattrs,
                             sshc->handle, &sshc->handleSz);
      if(rc == WS_FATAL_ERROR)
        rc = wolfSSH_get_error(sshc->ssh_session);
      if(rc == WS_WANT_READ) {
        *block = TRUE;
        conn->waitfor = KEEP_RECV;
        return CURLE_OK;
      }
      else if(rc == WS_WANT_WRITE) {
        *block = TRUE;
        conn->waitfor = KEEP_SEND;
        return CURLE_OK;
      }
      else if(rc == WS_SUCCESS) {
        infof(data, "wolfssh SFTP open succeeded");
      }
      else {
        failf(data, "wolfssh SFTP upload open failed: %d", rc);
        return CURLE_SSH;
      }
      wssh_state(data, sshc, SSH_SFTP_DOWNLOAD_STAT);

      /* If we have a restart point then we need to seek to the correct
         position. */
      if(data->state.resume_from > 0) {
        /* Let's read off the proper amount of bytes from the input. */
        int seekerr = CURL_SEEKFUNC_OK;
        if(data->set.seek_func) {
          Curl_set_in_callback(data, TRUE);
          seekerr = data->set.seek_func(data->set.seek_client,
                                        data->state.resume_from, SEEK_SET);
          Curl_set_in_callback(data, FALSE);
        }

        if(seekerr != CURL_SEEKFUNC_OK) {
          curl_off_t passed = 0;

          if(seekerr != CURL_SEEKFUNC_CANTSEEK) {
            failf(data, "Could not seek stream");
            return CURLE_FTP_COULDNT_USE_REST;
          }
          /* seekerr == CURL_SEEKFUNC_CANTSEEK (cannot seek to offset) */
          do {
            char scratch[4*1024];
            size_t readthisamountnow =
              (data->state.resume_from - passed >
                (curl_off_t)sizeof(scratch)) ?
              sizeof(scratch) : curlx_sotouz(data->state.resume_from - passed);

            size_t actuallyread;
            Curl_set_in_callback(data, TRUE);
            actuallyread = data->state.fread_func(scratch, 1,
                                                  readthisamountnow,
                                                  data->state.in);
            Curl_set_in_callback(data, FALSE);

            passed += actuallyread;
            if((actuallyread == 0) || (actuallyread > readthisamountnow)) {
              /* this checks for greater-than only to make sure that the
                 CURL_READFUNC_ABORT return code still aborts */
              failf(data, "Failed to read data");
              return CURLE_FTP_COULDNT_USE_REST;
            }
          } while(passed < data->state.resume_from);
        }

        /* now, decrease the size of the read */
        if(data->state.infilesize > 0) {
          data->state.infilesize -= data->state.resume_from;
          data->req.size = data->state.infilesize;
          Curl_pgrsSetUploadSize(data, data->state.infilesize);
        }

        sshc->offset += data->state.resume_from;
      }
      if(data->state.infilesize > 0) {
        data->req.size = data->state.infilesize;
        Curl_pgrsSetUploadSize(data, data->state.infilesize);
      }
      /* upload data */
      Curl_xfer_setup1(data, CURL_XFER_SEND, -1, FALSE);

      /* not set by Curl_xfer_setup to preserve keepon bits */
      conn->sockfd = conn->writesockfd;

      if(result) {
        wssh_state(data, sshc, SSH_SFTP_CLOSE);
        sshc->actualcode = result;
      }
      else {
        /* store this original bitmask setup to use later on if we cannot
           figure out a "real" bitmask */
        sshc->orig_waitfor = data->req.keepon;

        /* since we do not really wait for anything at this point, we want the
           state machine to move on as soon as possible */
        Curl_multi_mark_dirty(data);

        wssh_state(data, sshc, SSH_STOP);
      }
      break;
    }
    case SSH_SFTP_DOWNLOAD_INIT:
      sshc->handleSz = sizeof(sshc->handle);
      rc = wolfSSH_SFTP_Open(sshc->ssh_session, sftp_scp->path,
                             WOLFSSH_FXF_READ, NULL,
                             sshc->handle, &sshc->handleSz);
      if(rc == WS_FATAL_ERROR)
        rc = wolfSSH_get_error(sshc->ssh_session);
      if(rc == WS_WANT_READ) {
        *block = TRUE;
        conn->waitfor = KEEP_RECV;
        return CURLE_OK;
      }
      else if(rc == WS_WANT_WRITE) {
        *block = TRUE;
        conn->waitfor = KEEP_SEND;
        return CURLE_OK;
      }
      else if(rc == WS_SUCCESS) {
        infof(data, "wolfssh SFTP open succeeded");
        wssh_state(data, sshc, SSH_SFTP_DOWNLOAD_STAT);
        return CURLE_OK;
      }

      failf(data, "wolfssh SFTP open failed: %d", rc);
      return CURLE_SSH;

    case SSH_SFTP_DOWNLOAD_STAT: {
      WS_SFTP_FILEATRB attrs;
      curl_off_t size;

      rc = wolfSSH_SFTP_STAT(sshc->ssh_session, sftp_scp->path, &attrs);
      if(rc == WS_FATAL_ERROR)
        rc = wolfSSH_get_error(sshc->ssh_session);
      if(rc == WS_WANT_READ) {
        *block = TRUE;
        conn->waitfor = KEEP_RECV;
        return CURLE_OK;
      }
      else if(rc == WS_WANT_WRITE) {
        *block = TRUE;
        conn->waitfor = KEEP_SEND;
        return CURLE_OK;
      }
      else if(rc == WS_SUCCESS) {
        infof(data, "wolfssh STAT succeeded");
      }
      else {
        failf(data, "wolfssh SFTP open failed: %d", rc);
        data->req.size = -1;
        data->req.maxdownload = -1;
        Curl_pgrsSetDownloadSize(data, -1);
        return CURLE_SSH;
      }

      size = ((curl_off_t)attrs.sz[1] << 32) | attrs.sz[0];

      data->req.size = size;
      data->req.maxdownload = size;
      Curl_pgrsSetDownloadSize(data, size);

      infof(data, "SFTP download %" FMT_OFF_T " bytes", size);

      /* We cannot seek with wolfSSH so resuming and range requests are not
         possible */
      if(data->state.use_range || data->state.resume_from) {
        infof(data, "wolfSSH cannot do range/seek on SFTP");
        return CURLE_BAD_DOWNLOAD_RESUME;
      }

      /* Setup the actual download */
      if(data->req.size == 0) {
        /* no data to transfer */
        Curl_xfer_setup_nop(data);
        infof(data, "File already completely downloaded");
        wssh_state(data, sshc, SSH_STOP);
        break;
      }
      Curl_xfer_setup1(data, CURL_XFER_RECV, data->req.size, FALSE);

      /* not set by Curl_xfer_setup to preserve keepon bits */
      conn->writesockfd = conn->sockfd;

      if(result) {
        /* this should never occur; the close state should be entered
           at the time the error occurs */
        wssh_state(data, sshc, SSH_SFTP_CLOSE);
        sshc->actualcode = result;
      }
      else {
        wssh_state(data, sshc, SSH_STOP);
      }
      break;
    }
    case SSH_SFTP_CLOSE:
      if(sshc->handleSz) {
        rc = wolfSSH_SFTP_Close(sshc->ssh_session, sshc->handle,
                                sshc->handleSz);
        if(rc != WS_SUCCESS)
          rc = wolfSSH_get_error(sshc->ssh_session);
      }
      else {
        rc = WS_SUCCESS; /* directory listing */
      }
      if(rc == WS_WANT_READ) {
        *block = TRUE;
        conn->waitfor = KEEP_RECV;
        return CURLE_OK;
      }
      else if(rc == WS_WANT_WRITE) {
        *block = TRUE;
        conn->waitfor = KEEP_SEND;
        return CURLE_OK;
      }
      else if(rc == WS_SUCCESS) {
        wssh_state(data, sshc, SSH_STOP);
        return CURLE_OK;
      }

      failf(data, "wolfssh SFTP CLOSE failed: %d", rc);
      return CURLE_SSH;

    case SSH_SFTP_READDIR_INIT:
      Curl_pgrsSetDownloadSize(data, -1);
      if(data->req.no_body) {
        wssh_state(data, sshc, SSH_STOP);
        break;
      }
      wssh_state(data, sshc, SSH_SFTP_READDIR);
      break;

    case SSH_SFTP_READDIR:
      name = wolfSSH_SFTP_LS(sshc->ssh_session, sftp_scp->path);
      if(!name)
        rc = wolfSSH_get_error(sshc->ssh_session);
      else
        rc = WS_SUCCESS;

      if(rc == WS_WANT_READ) {
        *block = TRUE;
        conn->waitfor = KEEP_RECV;
        return CURLE_OK;
      }
      else if(rc == WS_WANT_WRITE) {
        *block = TRUE;
        conn->waitfor = KEEP_SEND;
        return CURLE_OK;
      }
      else if(name && (rc == WS_SUCCESS)) {
        WS_SFTPNAME *origname = name;
        result = CURLE_OK;
        while(name) {
          char *line = aprintf("%s\n",
                               data->set.list_only ?
                               name->fName : name->lName);
          if(!line) {
            wssh_state(data, sshc, SSH_SFTP_CLOSE);
            sshc->actualcode = CURLE_OUT_OF_MEMORY;
            break;
          }
          result = Curl_client_write(data, CLIENTWRITE_BODY,
                                     line, strlen(line));
          free(line);
          if(result) {
            sshc->actualcode = result;
            break;
          }
          name = name->next;
        }
        wolfSSH_SFTPNAME_list_free(origname);
        wssh_state(data, sshc, SSH_STOP);
        return result;
      }
      failf(data, "wolfssh SFTP ls failed: %d", rc);
      return CURLE_SSH;

    case SSH_SFTP_SHUTDOWN:
      wssh_sshc_cleanup(sshc);
      wssh_state(data, sshc, SSH_STOP);
      return CURLE_OK;
    default:
      break;
    }
  } while(!rc && (sshc->state != SSH_STOP));
  return result;
}

/* called repeatedly until done from multi.c */
static CURLcode wssh_multi_statemach(struct Curl_easy *data, bool *done)
{
  struct connectdata *conn = data->conn;
  struct ssh_conn *sshc = Curl_conn_meta_get(conn, CURL_META_SSH_CONN);
  CURLcode result = CURLE_OK;
  bool block; /* we store the status and use that to provide a ssh_getsock()
                 implementation */
  if(!sshc)
    return CURLE_FAILED_INIT;

  do {
    result = wssh_statemach_act(data, sshc, &block);
    *done = (sshc->state == SSH_STOP);
    /* if there is no error, it is not done and it did not EWOULDBLOCK, then
       try again */
    if(*done) {
      DEBUGF(infof(data, "wssh_statemach_act says DONE"));
    }
  } while(!result && !*done && !block);

  return result;
}

static
CURLcode wscp_perform(struct Curl_easy *data,
                      bool *connected,
                      bool *dophase_done)
{
  (void)data;
  (void)connected;
  (void)dophase_done;
  return CURLE_OK;
}

static
CURLcode wsftp_perform(struct Curl_easy *data,
                       struct ssh_conn *sshc,
                       bool *connected,
                       bool *dophase_done)
{
  CURLcode result = CURLE_OK;

  DEBUGF(infof(data, "DO phase starts"));

  *dophase_done = FALSE; /* not done yet */

  /* start the first command in the DO phase */
  wssh_state(data, sshc, SSH_SFTP_QUOTE_INIT);

  /* run the state-machine */
  result = wssh_multi_statemach(data, dophase_done);

  *connected = Curl_conn_is_connected(data->conn, FIRSTSOCKET);

  if(*dophase_done) {
    DEBUGF(infof(data, "DO phase is complete"));
  }

  return result;
}

/*
 * The DO function is generic for both protocols.
 */
static CURLcode wssh_do(struct Curl_easy *data, bool *done)
{
  CURLcode result;
  bool connected = FALSE;
  struct connectdata *conn = data->conn;
  struct ssh_conn *sshc = Curl_conn_meta_get(conn, CURL_META_SSH_CONN);

  *done = FALSE; /* default to false */
  if(!sshc)
    return CURLE_FAILED_INIT;

  data->req.size = -1; /* make sure this is unknown at this point */
  sshc->actualcode = CURLE_OK; /* reset error code */
  sshc->secondCreateDirs = 0;   /* reset the create dir attempt state
                                   variable */

  Curl_pgrsSetUploadCounter(data, 0);
  Curl_pgrsSetDownloadCounter(data, 0);
  Curl_pgrsSetUploadSize(data, -1);
  Curl_pgrsSetDownloadSize(data, -1);

  if(conn->handler->protocol & CURLPROTO_SCP)
    result = wscp_perform(data, &connected,  done);
  else
    result = wsftp_perform(data, sshc, &connected,  done);

  return result;
}

static CURLcode wssh_block_statemach(struct Curl_easy *data,
                                     struct ssh_conn *sshc,
                                     bool disconnect)
{
  struct connectdata *conn = data->conn;
  CURLcode result = CURLE_OK;

  while((sshc->state != SSH_STOP) && !result) {
    bool block;
    timediff_t left = 1000;
    struct curltime now = curlx_now();

    result = wssh_statemach_act(data, sshc, &block);
    if(result)
      break;

    if(!disconnect) {
      if(Curl_pgrsUpdate(data))
        return CURLE_ABORTED_BY_CALLBACK;

      result = Curl_speedcheck(data, now);
      if(result)
        break;

      left = Curl_timeleft(data, NULL, FALSE);
      if(left < 0) {
        failf(data, "Operation timed out");
        return CURLE_OPERATION_TIMEDOUT;
      }
    }

    if(!result) {
      int dir = conn->waitfor;
      curl_socket_t sock = conn->sock[FIRSTSOCKET];
      curl_socket_t fd_read = CURL_SOCKET_BAD;
      curl_socket_t fd_write = CURL_SOCKET_BAD;
      if(dir == KEEP_RECV)
        fd_read = sock;
      else if(dir == KEEP_SEND)
        fd_write = sock;

      /* wait for the socket to become ready */
      (void)Curl_socket_check(fd_read, CURL_SOCKET_BAD, fd_write,
                              left > 1000 ? 1000 : left); /* ignore result */
    }
  }

  return result;
}

/* generic done function for both SCP and SFTP called from their specific
   done functions */
static CURLcode wssh_done(struct Curl_easy *data,
                          struct ssh_conn *sshc,
                          CURLcode status)
{
  CURLcode result = CURLE_OK;

  if(!status) {
    /* run the state-machine */
    result = wssh_block_statemach(data, sshc, FALSE);
  }
  else
    result = status;

  if(Curl_pgrsDone(data))
    return CURLE_ABORTED_BY_CALLBACK;

  data->req.keepon = 0; /* clear all bits */
  return result;
}

static void wssh_sshc_cleanup(struct ssh_conn *sshc)
{
  if(sshc->ssh_session) {
    wolfSSH_free(sshc->ssh_session);
    sshc->ssh_session = NULL;
  }
  if(sshc->ctx) {
    wolfSSH_CTX_free(sshc->ctx);
    sshc->ctx = NULL;
  }
  Curl_safefree(sshc->homedir);
}

#if 0
static CURLcode wscp_done(struct Curl_easy *data,
                          CURLcode code, bool premature)
{
  CURLcode result = CURLE_OK;
  (void)conn;
  (void)code;
  (void)premature;

  return result;
}

static CURLcode wscp_doing(struct Curl_easy *data,
                           bool *dophase_done)
{
  CURLcode result = CURLE_OK;
  (void)conn;
  (void)dophase_done;

  return result;
}

static CURLcode wscp_disconnect(struct Curl_easy *data,
                                struct connectdata *conn, bool dead_connection)
{
  struct ssh_conn *sshc = Curl_conn_meta_get(conn, CURL_META_SSH_CONN);
  CURLcode result = CURLE_OK;
  (void)dead_connection;
  if(sshc)
    wssh_sshc_cleanup(sshc);
  return result;
}
#endif

static CURLcode wsftp_done(struct Curl_easy *data,
                           CURLcode code, bool premature)
{
  struct ssh_conn *sshc = Curl_conn_meta_get(data->conn, CURL_META_SSH_CONN);
  (void)premature;
  if(!sshc)
    return CURLE_FAILED_INIT;

  wssh_state(data, sshc, SSH_SFTP_CLOSE);

  return wssh_done(data, sshc, code);
}

static CURLcode wsftp_doing(struct Curl_easy *data,
                            bool *dophase_done)
{
  CURLcode result = wssh_multi_statemach(data, dophase_done);

  if(*dophase_done) {
    DEBUGF(infof(data, "DO phase is complete"));
  }
  return result;
}

static CURLcode wsftp_disconnect(struct Curl_easy *data,
                                 struct connectdata *conn,
                                 bool dead)
{
  struct ssh_conn *sshc = Curl_conn_meta_get(conn, CURL_META_SSH_CONN);
  CURLcode result = CURLE_OK;
  (void)dead;

  DEBUGF(infof(data, "SSH DISCONNECT starts now"));

  if(sshc && sshc->ssh_session) {
    /* only if there is a session still around to use! */
    wssh_state(data, sshc, SSH_SFTP_SHUTDOWN);
    result = wssh_block_statemach(data, sshc, TRUE);
  }

  if(sshc)
    wssh_sshc_cleanup(sshc);
  DEBUGF(infof(data, "SSH DISCONNECT is done"));
  return result;
}

static int wssh_getsock(struct Curl_easy *data,
                        struct connectdata *conn,
                        curl_socket_t *sock)
{
  int bitmap = GETSOCK_BLANK;
  int dir = conn->waitfor;
  (void)data;
  sock[0] = conn->sock[FIRSTSOCKET];

  if(dir == KEEP_RECV)
    bitmap |= GETSOCK_READSOCK(FIRSTSOCKET);
  else if(dir == KEEP_SEND)
    bitmap |= GETSOCK_WRITESOCK(FIRSTSOCKET);

  return bitmap;
}

void Curl_ssh_version(char *buffer, size_t buflen)
{
  (void)msnprintf(buffer, buflen, "wolfssh/%s", LIBWOLFSSH_VERSION_STRING);
}

CURLcode Curl_ssh_init(void)
{
  if(WS_SUCCESS != wolfSSH_Init()) {
    DEBUGF(fprintf(stderr, "Error: wolfSSH_Init failed\n"));
    return CURLE_FAILED_INIT;
  }

  return CURLE_OK;
}
void Curl_ssh_cleanup(void)
{
  (void)wolfSSH_Cleanup();
}

#endif /* USE_WOLFSSH */
