#define BGCOLOR "#99ccff"
#define TITLE(_x) <h1>_x</h1>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
#ifdef _PAGE_
<title>Rockbox - _PAGE_</title>
#else
<title>Rockbox</title>
#endif
<meta name="author" content="Björn Stenberg, in Emacs">
#ifndef _PAGE_
<meta name="keywords" content="bjorn,stenberg,computer,programming,mtb,stockholm,software,sms,byta,bostad">
#endif
</head>
<body bgcolor=BGCOLOR text="black" link="blue" vlink="purple" alink="red">
#ifdef _LOGO_
_LOGO_
#else
<a href="/rockbox/"><img align="right" src="/rockbox/rockbox100.png" width=99 height=30></a>

TITLE(_PAGE_)
#endif

