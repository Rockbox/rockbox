#define BGCOLOR "#b6c6e5"
#define MENUBG "#6887bb"
#define TITLE(_x) <h1>_x</h1>

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
<link rel="STYLESHEET" type="text/css" href="/style.css">
<link rel="shortcut icon" href="/favicon.ico">
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
&nbsp;<a href="/"><img src="/rockbox100.png" width=99 height=30 border=0></a>&nbsp;<br>

<p align="right">
<a class="menulink" href="/">main page</a><br>
<a class="menulink" href="/download/">download</a><br>
<a class="menulink" href="/manual/">manual</a><br>
<a class="menulink" href="/docs/faq.html">FAQ</a><br>
<a class="menulink" href="/tshirt-contest/">t-shirt contest</a><br>
<a class="menulink" href="/notes.html">research notes</a><br>
<a class="menulink" href="/docs/">data sheets</a><br>
<a class="menulink" href="/schematics/">schematics</a><br>
<a class="menulink" href="/mods/">hardware mods</a><br>
<a class="menulink" href="/mail/">mailing lists</a><br>
<a class="menulink" href="/irc/">IRC</a><br>
<a class="menulink" href="/tools.html">tools</a><br>
<a class="menulink" href="/internals/">photos</a><br>
<a class="menulink" href="/daily.shtml">daily builds</a><br>
<a class="menulink" href="/cvs.html">CVS</a><br>
<a class="menulink" href="http://sourceforge.net/projects/rockbox/">sourceforge</a><br>
<a class="menulink" href="http://sourceforge.net/tracker/?atid=439118&group_id=44306&func=browse">bug reports</a><br>
<a class="menulink" href="http://sourceforge.net/tracker/?atid=439121&group_id=44306&func=browse">feature requests</a><br>
<a class="menulink" href="http://bjorn.haxx.se/isd200/">linux driver</a>

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
