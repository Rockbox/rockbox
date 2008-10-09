<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
    <link href="layout.css" rel="stylesheet" type="text/css" />
    <link rel="STYLESHEET" type="text/css" href="http://www.rockbox.org/style.css">
    <link rel="shortcut icon" href="http://www.rockbox.org/favicon.ico">
    <title>Rockbox Plugin API - %GROUP_NAME%</title>
    <meta name="author" content="Rockbox Contributors">
</head>

<body>
    <table border=0 cellpadding=7 cellspacing=0>
    <tr valign="top">
    <td bgcolor="#6887bb" valign="top">
    <br>
    <div align="center">
    <a href="http://www.rockbox.org/">
    <img src="http://www.rockbox.org/rockbox100.png" width=99 height=30 border=0 alt="Rockbox.org home"></a>
    </div>
    <div style="margin-top:20px">
    <div class="submenu">
    Downloads
    </div>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/package.png' align='top'> <a class="menulink" href="http://www.rockbox.org/download/">releases</a><br>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/bomb.png' align='top'> <a class="menulink" href="http://build.rockbox.org">current build</a><br>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/style.png' align='top'> <a class="menulink" href="http://www.rockbox.org/twiki/bin/view/Main/RockboxExtras">extras</a>
    <div class="submenu">
    Documentation

    </div>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/help.png' align='top'> <a class="menulink" href="http://www.rockbox.org/twiki/bin/view/Main/GeneralFAQ">FAQ</a><br>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/page_white_acrobat.png' align='top'> <a class="menulink" href="http://www.rockbox.org/manual.shtml">manual</a><br>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/application_edit.png' align='top'> <a class="menulink" href="http://www.rockbox.org/twiki/">wiki</a><br>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/book_open.png' align='top'> <a class="menulink" href="http://www.rockbox.org/twiki/bin/view/Main/DocsIndex">docs index</a>
    <div class="submenu">
    Support
    </div>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/email.png' align='top'> <a class="menulink" href="http://www.rockbox.org/mail/">mailing lists</a><br>

    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/group.png' align='top'> <a class="menulink" href="http://www.rockbox.org/irc/">IRC</a><br>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/comment_edit.png' align='top'> <a class="menulink" href="http://forums.rockbox.org/">forums</a>
    <div class="submenu">
    Tracker
    </div>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/bug.png' align='top'> <a class="menulink" href="http://www.rockbox.org/tracker/index.php?type=2">bugs</a><br>
    <img width=16 height=16 src='http://www.rockbox.org/silk_icons/brick.png' align='top'> <a class="menulink" href="http://www.rockbox.org/tracker/index.php?type=4">patches</a><br>
    <div class="submenu">
    Search
    </div>

    <form id="fsform" action="http://www.rockbox.org/tracker/index.php" method="get" onSubmit="return fsstrip();">
    <input id="taskid" name="show_task" type="text" size="10" maxlength="10" accesskey="t"><br>
    <input class="mainbutton" type="submit" value="Flyspray #">
    </form>
    <br>
    <form action="http://www.google.com/search">
    <input name=as_q size=10><br>
    <input value="Search" type=submit>
    <input type=hidden name=as_sitesearch value="www.rockbox.org">
    </form>
    <p><form action="https://www.paypal.com/cgi-bin/webscr" method="get">
    <input type="hidden" name="cmd" value="_xclick">
    <input type="hidden" name="business" value="bjorn@haxx.se">
    <input type="hidden" name="item_name" value="Donation to the Rockbox project">
    <input type="hidden" name="no_shipping" value="1">
    <input type="hidden" name="cn" value="Note to the Rockbox team">
    <input type="hidden" name="currency_code" value="USD">

    <input type="hidden" name="tax" value="0">
    <input type="image" src="http://www.rockbox.org/paypal-donate.gif" name="submit">
    </form>
    </div>
    </td>
    <td class="plugincontent">
        <div id="menu">%MENU%</div>
        <div id="content">
            <a id="top"></a>
            <h2>%GROUP_NAME%</h2>
            <span class="group">
                %FUNCTION_START%
                <a id="%FUNCTION_NAME%"></a>
                <h3>%FUNCTION%</h3>
                
                %DESCRIPTION_START%
                %FUNCTION_DESCRIPTION%
                <br /><br />
                %DESCRIPTION_END%
                
                %PARAMETER_START%
                <span class="extra">Parameters:</span>
                <dl>
                    %PARAMETERS_START%
                    <dt>%PARAM1%</dt><dd>%PARAM2%</dd>
                    %PARAMETERS_END%
                </dl>
                %PARAMETER_END%
                
                %RETURN_START%
                <span class="extra">Returns:</span> %RETURN%<br /><br />
                %RETURN_END%
                
                %CONDITIONS_START%
                <span class="extra">Conditions:</span> %CONDITIONS%<br /><br />
                %CONDITIONS_END%
                
                %SEE_START%
                <span class="see">Also see %SEE%</span><br /><br />
                %SEE_END%
                
                <a href="#top" class="top">To top</a><hr />
                %FUNCTION_END%
            </span>
        </div>
    </td>
    </tr>
    </table>
</body>
</html>