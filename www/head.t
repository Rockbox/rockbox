#define BGCOLOR "#b6c6e5"
#define MENUBG "#6887bb"
#define TITLE(_x) <h1>_x</h1>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<link rel="STYLESHEET" type="text/css" href="/rockbox/style.css">
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
<body bgcolor=BGCOLOR text="black" link="blue" vlink="purple" alink="red"
      topmargin=0 leftmargin=0 marginwidth=0 marginheight=0>

<table border=0 cellpadding=7 cellspacing=0 height="100%">
<tr valign="top">
<td bgcolor=MENUBG valign="top">
<br>
&nbsp;<a href="/rockbox/"><img src="/rockbox/rockbox100.png" width=99 height=30></a>&nbsp;<br>

<p align="right">
<a class="menulink" href="/rockbox/">Main page</a><br>
<a class="menulink" href="/rockbox/docs/FAQ">FAQ</a><br>
<a class="menulink" href="/rockbox/notes.html">research notes</a><br>
<a class="menulink" href="/rockbox/docs/">data sheets</a><br>
<a class="menulink" href="/rockbox/schematics/">schematics</a><br>
<a class="menulink" href="/rockbox/mods/">hardware mods</a><br>
<a class="menulink" href="http://bjorn.haxx.se/rockbox/mail.cgi">mail list archive</a><br>
<a class="menulink" href="/rockbox/irc/">IRC</a><br>
<a class="menulink" href="/rockbox/tools.html">tools</a><br>
<a class="menulink" href="/rockbox/internals/">photos</a><br>
<a class="menulink" href="/cvs.html">CVS</a><br>
<a class="menulink" href="http://sourceforge.net/projects/rockbox/">sourceforge</a><br>
<a class="menulink" href="/isd200/">linux driver</a>

<div align="right">
<form action="http://www.google.com/search">
<input name=as_q size=12><br>
<input value="Search" type=submit>
<input type=hidden name=as_oq value=rockbox>
<input type=hidden name=as_sitesearch value="bjorn.haxx.se">
</form></div>

</td>
<td>

#ifdef _LOGO_
<div align="center">_LOGO_</div>
#else
TITLE(_PAGE_)
#endif
