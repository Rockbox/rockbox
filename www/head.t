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
<a class="menulink" href="/">home</a><br>
<a class="menulink" href="/docs/features.html">features</a><br>
<a class="menulink" href="/download/">download</a><br>
<a class="menulink" href="/screenshots.html">screen&nbsp;shots</a><br>
<a class="menulink" href="/docs/">documentation</a><br>
<a class="menulink" href="/mail/">mailing lists</a><br>
<a class="menulink" href="/irc/">IRC</a><br>
<a class="menulink" href="/internals/">photos</a><br>
<a class="menulink" href="/daily.shtml">daily builds</a><br>
<a class="menulink" href="/mods/">hardware mods</a><br>
<a class="menulink" href="/requests.shtml">feature&nbsp;requests</a><br>
<a class="menulink" href="/bugs.shtml">bug reports</a><br>
<a class="menulink" href="http://sourceforge.net/tracker/?group_id=44306&atid=439120">patches</a><br>
<a class="menulink" href="http://bjorn.haxx.se/isd200/">linux driver</a>

<div align="right">
<form action="http://www.google.com/search">
<input name=as_q size=12><br>
<input value="Search" type=submit>
<input type=hidden name=as_sitesearch value="rockbox.haxx.se">
</form>
<a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/rockbox/"><img src="/viewcvs.png" width="128" height="48" border="0" alt="View CVS"></a>

<br><a href="http://sourceforge.net/projects/rockbox/"><img src="http://sourceforge.net/sflogo.php?group_id=44306&amp;type=1" width="88" height="31" border="0" alt="SourceForge.net Logo"></a>

<p class=menutext>Bandwidth and server<br>sponsored by:<br>
<a href="http://www.contactor.se"><img border=0 src="/cont.png" width=101 height=36></a>

<form action="https://www.paypal.com/cgi-bin/webscr" method="post">
<input type="hidden" name="cmd" value="_xclick">
<input type="hidden" name="business" value="bjorn@haxx.se">
<input type="hidden" name="item_name" value="Donation to the Rockbox project">
<input type="hidden" name="no_shipping" value="1">
<input type="hidden" name="cn" value="Note to the Rockbox team">
<input type="hidden" name="currency_code" value="USD">
<input type="hidden" name="tax" value="0">
<input type="image" src="/paypal-donate.gif" border="0" name="submit" alt="Make payments with PayPal - it's fast, free and secure!">
</form>
</div>

</td>
<td>

#ifdef _LOGO_
<div align="center">_LOGO_</div>
#else
TITLE(_PAGE_)
#endif
