<?
$svn = "http://svn.rockbox.org/viewvc.cgi/trunk/";
$wiki = "http://www.rockbox.org/wiki/";

function func_sort($a, $b)
{
    $a = preg_replace('/^((unsigned|const|struct|enum) [^ ]*|[a-z0-9 \*_]*) [\*]?/i', '', $a);
    $b = preg_replace('/^((unsigned|const|struct|enum) [^ ]*|[a-z0-9 \*_]*) [\*]?/i', '', $b);
    return strnatcasecmp($a, $b);
}

function get_newest()
{
    global $svn;
    
    $text = file_get_contents("../../apps/plugin.h");

    $text = str_replace(array("\r\n", "\r"), "\n", $text);

    /* Located plugin_api struct */
    foreach(explode("\n", $text) as $line_nr => $line)
    {
        if(trim($line) == "struct plugin_api {")
        {
            $text = explode("\n", $text);
            $text = array_slice($text, $line_nr+1);
            break;
        }
    }

    foreach($text as $line_nr => $line)
    {
        if(trim($line) == "};")
        {
            $text = array_slice($text, 0, $line_nr-1);
            break;
        }
    }
    /* Locating done */

    /* Clean up stuff a bit .. */
    for($i=0; $i<count($text); $i++)
        $text[$i] = trim($text[$i]);


    /* Fake preprocesser */
    $ret = array();
    $_groups = array();
    $conditions = array();
    $strip_next = 0;
    $group = "";
    for($i=0; $i<count($text); $i++)
    {
        $tmp = trim($text[$i]);
        
        if(substr($tmp, 0, 1) == '#')
        {
            $tmp = trim(substr($tmp, 1));
            if(strtolower(substr($tmp, 0, 2)) == "if")
            {
                if(strtolower(substr($tmp, 2, 3)) == "def")
                    $conditions[] = "defined(".substr($tmp, 6).")";
                else if(strtolower(substr($tmp, 2, 4)) == "ndef")
                    $conditions[] = "!defined(".substr($tmp, 7).")";
                else
                {
                    while(substr($tmp, strlen($tmp)-1, 1) == "\\")
                    {
                       $i++;
                       $tmp = substr($tmp, 0, strlen($tmp)-1)." ".trim($text[$i]);
                    }

                    $conditions[] = substr($tmp, 3);
                }
            }
            else if(strtolower(substr($tmp, 0, 4)) == "elif")
            {
                while(substr($tmp, strlen($tmp)-1, 1) == "\\")
                {
                   $i++;
                   $tmp = substr($tmp, 0, strlen($tmp)-1)." ".trim($text[$i]);
                }
                $conditions[count($conditions)-1] = substr($tmp, 5);
            }
            else if(strtolower(substr($tmp, 0, 4)) == "else")
                $conditions[count($conditions)-1] = "!( ".$conditions[count($conditions)-1]." )";
            else if(strtolower(substr($tmp, 0, 5)) == "endif")
                array_pop($conditions);
        }
        else if(strlen($tmp) == 0)
            $group = "";
        else if(substr($tmp, 0, 2) == "/*")
        {
            while(strpos($tmp, "*/") === false)
            {
               $i++;
               $tmp .= " ".trim($text[$i]);
            }
            $group = explode("/*", trim($tmp));
            $group = explode("*/", $group[1]);
            $group = trim($group[0]);
        }
        else
        {
            while(strpos($tmp, ";") === false)
            {
               $i++;
               $tmp .= " ".trim($text[$i]);
            }

            /* Replace those (*func)(int args) with func(int args) */
            $tmp = preg_replace('/\(\*([^\)]*)\)/i', '\1', $tmp, 1);
            $tmp = substr($tmp, 0, strlen($tmp)-1);
            $ret[$tmp] = array("func" => $tmp, "cond" => "(".implode(") && (", $conditions).")", "group" => $group);
        }
    }

    uksort($ret, "func_sort");

    return $ret;
}

function parse_documentation($data)
{
    $data = explode("\n", $data);

    $ret = array();
    $cur_func = "";
    foreach($data as $line)
    {
        if(substr($line, 0, 1) == "#")
            continue;
        else if(substr($line, 0, 4) == "    ")
        {
            $tmp = trim($line);
            if(strpos($tmp, " ") !== false)
                $tmp = array(substr($tmp, 1, strpos($tmp, " ")-1), substr($tmp, strpos($tmp, " ")) );
            else
                $tmp = array(substr($tmp, 1), "");

            $ret[$cur_func][$tmp[0]][] = $tmp[1];
        }
        else if(strlen($line) == 0)
            continue;
        else
            $cur_func = substr($line, 0);
    }

    $_ret = array();
    foreach($ret as $func => $el)
    {
        if(isset($el["group"]))
            $group = trim($el["group"][0]);
        else
            $group = "misc";

        $_ret[$group][$func] = $el;
    }

    return $_ret;
}

function get_func($func)
{
    $func = preg_replace('/^((unsigned|const|struct|enum) [^ ]*|[a-z0-9 \*_]*) [\*]?/i', '', $func);
    if(strpos($func, "PREFIX") !== false)
        $func = substr($func, 0, strrpos($func, "("));
    else if(strpos($func, "(") !== false)
        $func = substr($func, 0, strpos($func, "("));

    return $func;
}

function get_args($func)
{
    /* Check if this _is_ a function */
    if(strpos($func, "(") === false)
        return array();

    /* Get rid of return value */
    $func = preg_replace('/^((unsigned|const|struct|enum) [^ ]*|[a-z0-9 \*_]*) [\*]?/i', '', $func);

    /* Get rid of function name */
    if(strpos($func, "(") !== false)
        $func = substr($func, strpos($func, "("));

    /* Get rid of ATTRIBUTE_PRINTF */
    if(strpos($func, "ATTRIBUTE_PRINTF") !== false)
        $func = substr($func, 0, strpos($func, "ATTRIBUTE_PRINTF"));

    $level = 0;
    $args = array();
    $buffer = "";
    for($i=0; $i<strlen($func); $i++)
    {
        switch($func{$i})
        {
            case "(":
                $level++;
                if($level > 1)
                    $buffer .= "(";
            break;
            case ")":
                $level--;
                if($level > 0)
                {
                    $buffer .= ")";
                    break;
                }
            case ",":
                if($level <= 1)
                {
                    if(strpos($buffer, "(,") !== false)
                    {
                        $tmp = array();
                        preg_match_all('/[^ ]*, [^)]*\)/', $buffer, $tmp);
                        $tmp = $tmp[0];
                        foreach($tmp as $el)
                        {
                            if(strlen($el) > 0)
                                $args[] = trim($el);
                        }
                        $tmp = preg_replace('/[^ ]*, [^)]*\)/', '', $buffer);
                        $args[] = trim($tmp);
                    }
                    else
                        $args[] = trim($buffer);
                    $buffer = "";
                }
                else
                    $buffer .= ",";
            break;
            default:
                $buffer .= $func{$i};
            break;
        }
    }

    /* Filter out void */
    for($i=0; $i<count($args); $i++)
    {
        if($args[$i] == "void")
            unset($args[$i]);
    }

    return $args;
}

function get_return($func)
{
    $ret = array();
    preg_match('/^((unsigned|const|struct|enum) [^ ]*|[a-z0-9 \*_]*) [\*]?/i', $func, $ret);

    if(trim($ret[0]) == "void")
        return false;
    else
        return trim($ret[0]);
}

function split_var($var)
{
    if(strpos($var, "(,") !== false)
    {
        $p1 = substr($var, 0, strrpos($var, " "));
        $p2 = substr($var, strrpos($var, " "));
        $p2 = substr($p2, 0, strlen($p2)-1);
    }
    else if(strpos($var, "(*") !== false)
    {
        $p2 = array();
        preg_match('/\(\*\w*\)/', $var, $p2);
        $p2 = $p2[0];

        $p1 = substr($var, strpos($var, $p2));
        $p2 = substr($p2, 2, strlen($p2)-3);
    }
    else
    {
        $p1 = substr($var, 0, strrpos($var, " "));
        $p2 = substr($var, strrpos($var, " "));
    }

    if(strpos($p2, "*") !== false)
    {
       for($i=0; $i<substr_count($p2, "*"); $i++)
           $p1 .= "*";
       $p2 = str_replace("*", "", $p2);
    }

    return array(trim($p1), trim($p2));
}

function _simplify($text)
{
    $text = ereg_replace('\(!\( (.*)[ ]?\)\)', '!\1', $text);
    $text = ereg_replace('\(\(([^ ])\)\)', '\1', $text);
    return $text;
}

function clean_func($func)
{
    $func = str_replace(array("   ", "  "), " ", $func);
    $func = str_replace("  ", " ", $func);
    return $func;
}

function do_see_markup($data)
{
    $ret = array();
    foreach($data as $el)
    {
        $el = trim($el);
        
        if(substr($el, 0, 1) != "[")
           $ret[] = do_markup("[F[".$el."]]");
        else
           $ret[] = do_markup($el);
    }

    return implode(" &amp; ", $ret);
}

function do_markup($data)
{
    global $svn, $wiki;

    $data = ereg_replace('=([^=]*)=', '<code>\1</code>', $data);
    $data = ereg_replace('\[W\[([^#\[]*)([^\[]*)\]\]', '<a href="'.$wiki.'\1\2">\1</a>', $data);
    $data = ereg_replace('\[S\[([^\[]*)\]\]', '<a href="'.$svn.'\1?content-type=text%2Fplain">\1</a>', $data);
    $data = ereg_replace('\[F\[([^\[]*)\]\]', '<a href="#\1">\1</a>', $data);
    $data = ereg_replace('\[\[([^#\[]*)([^\[]*)\]\]', '<a href="\1\2">\1</a>', $data);
    $data = str_replace("%BR%", "<br />", $data);
    $data = nl2br($data);

    return $data;
}

function get_tpl_part($search, $haystack)
{
    $tpl = array();
    ereg($search[0].".*".$search[1], $haystack, $tpl);
    return str_replace(array($search[0], $search[1]), "", $tpl[0]);
}
?>
