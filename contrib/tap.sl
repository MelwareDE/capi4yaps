%	-*- slang -*-
%
%	(C) by Ulrich Dessauer
%
%	This is an example for scripting by re-implementing TAP
%	using SLang. This is a minimal implementation and has not
%	all the fancy features the internal version has, but it
%	should only be an example for the usage of scripting.
%
%
%	Add these lines if you want to use it in your yaps.rc
%	protocol	script
%	use-call-id	False
%	script-type	SLang
%	script-name	/path/to/this/file/tap.sl
%	scr-login	tap_login
%	scr-logout	tap_logout
%	scr-pagerid	tap_pagerid
%	scr-message	tap_message
%	scr-next	tap_next
%	scr-sync	tap_sync
%
variable	tap_pid = Null_String;
variable	tap_t1 = 2;
variable	tap_t3 = 10;

define
tap_login (callid)
{
	variable	n, ep;
	
	for (n = 0; n < 3; ++n) {
		!if (send ("\r"))
			return (ERR_FATAL);
		if (expect (tap_t1, "ID=", 1) == 1)
			break;
	}
	if (n == 3)
		return (ERR_FATAL);
	for (n = 0; n < 3; ++n) {
		!if (send ("\033PG1\r"))
			return (ERR_FATAL);
		ep = expect (tap_t3, "\x06\r", "\x15\r", "\x1b\x04\r", 3);
		if (ep == 1)
			break;
		else if ((ep == 2) or (ep == 3))
			return (ERR_ABORT);
	}
	if (n == 3)
		return (ERR_FATAL);
	if (expect (tap_t3, "\x1b[p\r", 1) != 1)
		return (ERR_ABORT);
	return (NO_ERR);
}

define
tap_logout (dummy)
{
	variable	n, ep;
	
	for (n = 0; n < 3; ++n) {
		!if (send ("\x04\r"))
			return (ERR_FATAL);
		ep = expect (tap_t3, "\x1b\x04\r", "\x1e\r", 2);
		if (ep == 1)
			break;
		else if (ep != 2)
			return (ERR_FATAL);
	}
	if (n == 3)
		return (ERR_FATAL);
	return (NO_ERR);
}

define
tap_pagerid (pid)
{
	tap_pid = conv (pid);
	return (NO_ERR);
}

define
tap_message (msg)
{
	variable	str;
	variable	chk;
	variable	n, ep, len;
	
	str = "\x02" + tap_pid + "\r" + conv (msg) + "\r" + "\x03";
	len = strlen (str);
	chk = 0;
	for (n = 0; n < len; ++n)
		chk = chk + (str[n] & 0xff);
	str = str + Sprintf ("%c%c%c\r", ((chk shr 8) & 0xf) + 0x30, ((chk shr 4) & 0xf) + 0x30, (chk & 0xf) + 0x30, 3);
	for (n = 0; n < 3; ++n) {
		!if (send (str))
			return (ERR_FATAL);
		ep = expect (tap_t3, "\x06\r", "\x15\r", "\x1e\r", "\x1b\x04\r", 4);
		if (ep == 1)
			break;
		else if ((ep == 2) or (ep == 3))
			return (ERR_FATAL);
		else if (ep == 4)
			return (ERR_ABORT);
	}
	if (n == 3)
		return (ERR_FATAL);
	return (NO_ERR);
}

define
tap_next (dummy)
{
	return (NO_ERR);
}

define
tap_sync (dummy)
{
	return (ERR_FATAL);
}
	
