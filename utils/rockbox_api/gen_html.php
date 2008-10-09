#!/usr/bin/php
<?
require_once("functions.php");

function get_group($text)
{
    return str_replace(array(" ", "/"), "_", $text);
}

$input = file_get_contents($argv[1]);

$mypath = $_SERVER['SCRIPT_FILENAME'];
$mypath = substr($mypath, 0, strrpos($mypath, "/"))."/";

$inh = parse_documentation($input);

@mkdir("output");

$index_tpl = file_get_contents($mypath."index.tpl");

$group_data = array();
$group_tpl = get_tpl_part(array("%GROUP_START%", "%GROUP_END%"), $index_tpl);
$func_tpl = get_tpl_part(array("%FUNCTION_START%", "%FUNCTION_END%"), $index_tpl);

foreach($inh as $group_name => $group)
{
    if(strlen($group_name) > 0)
    {
        $func_data = array();
        foreach($group as $el_name => $el)
            $func_data[] = str_replace(array("%GROUP%", "%FUNCTION%", "%FUNCTION_NAME%"),
                                       array(get_group($group_name), get_func($el_name), $el_name),
                                       $func_tpl);
        
        $tmp = str_replace("%GROUP_NAME%", ucwords($group_name), $group_tpl);
        $group_data[] = ereg_replace("%FUNCTION_START%.*%FUNCTION_END%", implode("\n", $func_data), $tmp);
    }
}

$index_tpl = ereg_replace("%GROUP_START%.*%GROUP_END%", implode("", $group_data), $index_tpl);
file_put_contents("output/index.html", $index_tpl);

$menu_tpl = file_get_contents($mypath."menu.tpl");
$group_tpl = get_tpl_part(array("%GROUP_START%", "%GROUP_END%"), $menu_tpl);

$menu = array();
foreach($inh as $group_name => $group)
{
    if(strlen($group_name) > 0)
        $menu[strtolower($group_name)] = str_replace(array("%GROUP%", "%GROUP_NAME%"),
                                                     array(get_group($group_name), ucwords($group_name)),
                                                     $group_tpl);
}
ksort($menu);

$menu = ereg_replace("%GROUP_START%.*%GROUP_END%", implode("", $menu), $menu_tpl);

$section_tpl = file_get_contents($mypath."section.tpl");

$func_tpl = get_tpl_part(array("%FUNCTION_START%", "%FUNCTION_END%"), $section_tpl);
$description_tpl = get_tpl_part(array("%DESCRIPTION_START%", "%DESCRIPTION_END%"), $section_tpl);
$parameter_tpl = get_tpl_part(array("%PARAMETER_START%", "%PARAMETER_END%"), $section_tpl);
$parameters_tpl = get_tpl_part(array("%PARAMETERS_START%", "%PARAMETERS_END%"), $section_tpl);
$return_tpl = get_tpl_part(array("%RETURN_START%", "%RETURN_END%"), $section_tpl);
$conditions_tpl = get_tpl_part(array("%CONDITIONS_START%", "%CONDITIONS_END%"), $section_tpl);
$see_tpl = get_tpl_part(array("%SEE_START%", "%SEE_END%"), $section_tpl);

foreach($inh as $group_name => $group)
{
    $section_data = str_replace(array("%MENU%", "%GROUP_NAME%"), array($menu, ucwords($group_name)), $section_tpl);
    
    $funcs_data = array();
    foreach($group as $func_name => $func)
    {
        $func_data = str_replace(array("%FUNCTION_NAME%", "%FUNCTION%"), array(get_func($func_name), $func_name), $func_tpl);
        
        if(strlen($func["description"][0]) > 0)
            $func_data = ereg_replace("%DESCRIPTION_START%.*%DESCRIPTION_END%",
                                          str_replace("%FUNCTION_DESCRIPTION%", do_markup($func["description"][0]), $description_tpl),
                                          $func_data);
        else
            $func_data = ereg_replace("%DESCRIPTION_START%.*%DESCRIPTION_END%", "", $func_data);
        
        if(isset($func["param"]))
        {
            $params_data = array();
            foreach($func["param"] as $param)
            {
                $param = trim($param);
                $p1 = substr($param, 0, strpos($param, " "));
                $p2 = do_markup(substr($param, strpos($param, " ")));
                
                if(strlen($p1) > 0 && strlen($p2) > 0)
                    $params_data[] = str_replace(array("%PARAM1%", "%PARAM2%"), array($p1, $p2), $parameters_tpl);
            }
            
            
            if(count($params_data) > 0)
                $func_data = ereg_replace("%PARAMETER_START%.*%PARAMETER_END%",
                                          ereg_replace("%PARAMETERS_START%.*%PARAMETERS_END%", implode("\n", $params_data), $parameter_tpl),
                                          $func_data);
            else
                $func_data = ereg_replace("%PARAMETER_START%.*%PARAMETER_END%", "", $func_data);
        }
        else
            $func_data = ereg_replace("%PARAMETER_START%.*%PARAMETER_END%", "", $func_data);

        if(isset($func["return"]) && strlen($func["return"][0]) > 0)
            $func_data = ereg_replace("%RETURN_START%.*%RETURN_END%",
                                      str_replace("%RETURN%", do_markup($func["return"][0]), $return_tpl),
                                      $func_data);
        else
            $func_data = ereg_replace("%RETURN_START%.*%RETURN_END%", "", $func_data);
        
        if(isset($func["conditions"]))
            $func_data = ereg_replace("%CONDITIONS_START%.*%CONDITIONS_END%",
                                      str_replace("%CONDITIONS%", $func["conditions"][0], $conditions_tpl),
                                      $func_data);
        else
            $func_data = ereg_replace("%CONDITIONS_START%.*%CONDITIONS_END%", "", $func_data);

        if(isset($func["see"]))
            $func_data = ereg_replace("%SEE_START%.*%SEE_END%",
                                      str_replace("%SEE%", do_see_markup(explode(" ", trim($func["see"][0]))), $see_tpl),
                                      $func_data);
        else
            $func_data = ereg_replace("%SEE_START%.*%SEE_END%", "", $func_data);
        
        $funcs_data[] = $func_data;
    }
    $section_data = ereg_replace("%FUNCTION_START%.*%FUNCTION_END%", implode("", $funcs_data), $section_data);
    
    file_put_contents("output/".get_group($group_name).".html", $section_data);
}

copy($mypath."layout.css", "output/layout.css");
?>