/*=============================================================================
    MsnSend.cpp - the senders of the messenger's connections (C3 only; C3 addresses): the text of a command (CRLF ended, in
    UTF-8) and a payload (as it is, in the ANSI code page) written to the socket. Stock: 0x1013BDC0, 0x1013BF30, 0x1013C040. They
    are members of MSNSocket (MSNSocket.h: Send / SendPayload call SendLine / SendRaw on a copy); the by-value command is an
    NMsnLocalCommand, whose destructor is the stock's Release (0x1012A0E0) that the callee runs at its end.
=============================================================================*/
#include "nwindow/MSNSocket.h"
#include "nwindow/MsnCommand.h"
#include "nwindow/MsnUtil.h"
#include "nwindow/NChatMsg.h"

#include <string.h>

// winsock: the stock image's own import thunk in the hook build (stdcall; the hook DLL does not import ws2_32), the import itself in
// the standalone (NMsnProtocol.h includes winsock2.h there)
static FORCEINLINE INT MsnSend(UINT Socket, const char* Buffer, INT Length, INT Flags)    // thunk 0x1014C174
{
#if NW_HOOK_MODE
    typedef INT(__stdcall * Fn)(UINT, const char*, INT, INT);
    return ((Fn)::nw::Addr(0x1014C174))(Socket, Buffer, Length, Flags);
#else
    return send(Socket, Buffer, Length, Flags);
#endif
}

// 0x1013bdc0 (ret 0x1C) - a command with something in it: its wire text and CR LF, converted to UTF-8 (0x101442A0), is sent; a short
// write reads the socket's error (nothing is done with it). Whether it had anything or not, the transaction counter goes up and the
// command is released (the parameter's destructor: NMsnLocalCommand, the stock's Release 0x1012A0E0)
void MSNSocket::SendLine(NMsnLocalCommand Command)
{
    if (Command.GetLength() > 0)
    {
        FString Text;
        {
            FString Line = Command.ToString();
            Text = Line + TEXT("\r\n");
        }
        char Buffer[0x410];
        appMemset(Buffer, 0, 0x410);
        NUtf8Encode(*Text, Buffer);
        INT Length = (INT)strlen(Buffer);
        if (MsnSend(Sock.hSock, Buffer, Length, 0) != (INT)strlen(Buffer))
            MsnWSAGetLastError();
    }
    NextTrID++;
}
NW_HOOK_VARARGS(0x1013BDC0, MSNSocket, SendLine, void (MSNSocket::*)(NMsnLocalCommand))

// 0x1013bf30 (ret 0x1C) - the same for a payload: the wire text without CR LF, in the ANSI code page (0x10144220)
void MSNSocket::SendRaw(NMsnLocalCommand Command)
{
    if (Command.GetLength() > 0)
    {
        FString Text = Command.ToString();
        char Buffer[0x410];
        appMemset(Buffer, 0, 0x410);
        MsnWideToAnsi(*Text, Buffer);
        INT Length = (INT)strlen(Buffer);
        if (MsnSend(Sock.hSock, Buffer, Length, 0) != (INT)strlen(Buffer))
            MsnWSAGetLastError();
    }
    NextTrID++;
}
NW_HOOK_VARARGS(0x1013BF30, MSNSocket, SendRaw, void (MSNSocket::*)(NMsnLocalCommand))

// 0x1013c040 (ret 0x74) - a command followed by a message: the message's payload (0x10127980: its headers and text) converted to UTF-8
// (0x101442A0) gives the byte count, which becomes the command's last argument (0x10129810); the line (the wire text, CR LF) and the
// payload are joined, converted and sent in one write (a short write reads the socket's error); the transaction counter goes up. The command is
// released (0x1012A0E0) and the message destroyed at the end (the parameters' destructors)
void MSNSocket::SendMessageBody(NMsnLocalCommand Command, NMsnMessage Message)
{
    FString Payload = Message.Serialize();
    FString Text;
    char Buffer[0x1040];
    appMemset(Buffer, 0, 0x1040);
    NUtf8Encode(*Payload, Buffer);
    INT Length = (INT)strlen(Buffer);
    Command.AddIntArg(Length);
    {
        FString Line = Command.ToString();
        Text = Line;
    }
    Text += TEXT("\r\n");
    Text += Payload;
    appMemset(Buffer, 0, 0x1040);
    NUtf8Encode(*Text, Buffer);
    if (MsnSend(Sock.hSock, Buffer, (INT)strlen(Buffer), 0) != (INT)strlen(Buffer))
        MsnWSAGetLastError();
    NextTrID++;
}
typedef void (MSNSocket::*SigSendMessageBody)(NMsnLocalCommand, NMsnMessage);
NW_HOOK_VARARGS(0x1013C040, MSNSocket, SendMessageBody, SigSendMessageBody)
