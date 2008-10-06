#!/usr/bin/php
<?
require_once("functions.php");

function get_group($text)
{
    return str_replace(array(" ", "/"), "_", $text);
}

$input = file_get_contents($argv[1]);

$inh = parse_documentation($input);

@mkdir("output");

$h = fopen("output/index.html", "w");

fwrite($h, '<html><head><link href="layout.css" rel="stylesheet" type="text/css" /><title>Plugin API - INDEX</title></head><body>');

fwrite($h, "<h1>Plugin API reference</h1>");
fwrite($h, "<ul>");

foreach($inh as $group_name => $group)
{
    if(strlen($group_name) > 0)
    {
        fwrite($h, '<li>'.ucwords($group_name)."<ul>");
        
        foreach($group as $el_name => $el)
            fwrite($h, "<li><a href=\"".get_group($group_name).".html#".get_func($el_name)."\">".$el_name."</a></li>");
        
        fwrite($h, "</ul></li>");
    }
}
fwrite($h, "</ul></body></html>");

fclose($h);

$menu = '<ul><li><a href="index.html">INDEX</a></li><ul>';
$_menu = array();
foreach($inh as $group_name => $group)
{
    if(strlen($group_name) > 0)
        $_menu[strtolower($group_name)] = '<li><a href="'.get_group($group_name).'.html">'.ucwords($group_name).'</a></li>';
}

ksort($_menu);
$menu .= implode("\n", $_menu);
$menu .= "</ul></ul>";

foreach($inh as $group_name => $group)
{
    $h = fopen("output/".get_group($group_name).".html", "w");

    fwrite($h, '<html><head><link href="layout.css" rel="stylesheet" type="text/css" /><title>Plugin API - '.ucwords($group_name).'</title></head><body>');
    fwrite($h, '<div id="menu">'.ucwords($menu).'</div>');
    fwrite($h, '<div id="content">');
    fwrite($h, '<a link="top"></a>');

    fwrite($h, "<h2>".ucwords($group_name)."</h2>");
    fwrite($h, '<span class="group">');
    foreach($group as $func_name => $func)
    {
        fwrite($h, '<a id="'.get_func($func_name).'"></a>');

        fwrite($h, "<h3>$func_name</h3>");
        
        if(strlen($func["description"][0]) > 0)
            fwrite($h, do_markup($func["description"][0])."<br /><br />");

        if(isset($func["param"]))
        {
            $params = "";
            foreach($func["param"] as $param)
            {
                $param = trim($param);
                $p1 = substr($param, 0, strpos($param, " "));
                $p2 = substr($param, strpos($param, " "));
                if(strlen($p1) > 0 && strlen($p2) > 0)
                    $params .= '<dt>'.$p1.'</dt><dd> '.do_markup($p2).'</dd>';
            }
            
            if(strlen($params) > 0)
            {
                fwrite($h, '<span class="extra">Parameters:</span><dl>');
                fwrite($h, $params);
                fwrite($h, "</dl>");
            }
        }

        if(isset($func["return"]) && strlen($func["return"][0]) > 0)
            fwrite($h, '<span class="extra">Returns:</span> '.do_markup($func["return"][0]).'<br /><br />');
        
        if(isset($func["conditions"]))
            fwrite($h, '<span class="extra">Conditions:</span> '.$func["conditions"][0].'<br /><br />');

        if(isset($func["see"]))
            fwrite($h, '<span class="see">Also see '.do_see_markup(explode(" ", trim($func["see"][0]))).'</span><br /><br />');

        fwrite($h, '<a href="#top" class="top">To top</a><hr />');
    }
    fwrite($h, "</span>");
    
    fwrite($h, "</div></body></html>");
    
    fclose($h);
}

copy("layout.css", "output/layout.css");
?>