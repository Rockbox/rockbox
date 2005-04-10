#ifndef MAKE_RSS
#ifdef MAKE_MAIL
#define ZAGOR  Björn Stenberg
#define BAGDER Daniel Stenberg
#define LINUSN Linus Nielsen Feltzing

#define NEWSDATE(x) Date: x
#define ITEM ---
#define NAME(x) x
#define ENDDATE
#define LINK(url,name) [URL]url[URL] [TEXT]name[TEXT]

#else
#define _PAGE_ Rockbox Digest
#include "head.t"
#include "news.t"

<small>
<a href="./">digest front page</a>
&middot;
<a href="./digest.rss">digest RSS feed</a>
</small>
<p>
#endif
#else
<?xml version="1.0"?>
<!DOCTYPE rss PUBLIC "-//Netscape Communications//DTD RSS 0.91//EN" "http://my.netscape.com/publish/formats/rss-0.91.dtd">
<rss version="0.91">
<channel>
<title>Rockbox Digest</title>
<link>http://rockbox.haxx.se/digest/digest.html</link>
<description>Detailing the latest and the most significant subjects about Rockbox.</description>
<language>en</language>

#define NEWSDATE(x) <item><title>x</title><link>http://rockbox.haxx.se/digest/digest.html</link> <description>  &lt;ol&gt;
#define ENDDATE &lt;/ol&gt;  </description></item>
#define ITEM &lt;li&gt;
#define NAME(x) x

#define ZAGOR  Bj&ouml;rn Stenberg
#define BAGDER Daniel Stenberg
#define LINUSN Linus Nielsen Feltzing

#define LINK(url, name) &lt;a href=url&gt;name&lt;/a&gt;
#endif
