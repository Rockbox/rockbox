<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<link href="layout.css" rel="stylesheet" type="text/css" />
<title>Plugin API - %GROUP_NAME%</title>
</head>

<body>
    <div id="menu">%MENU%</div>
    <div id="content">
        <a link="top"></a>
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
</body>
</html>