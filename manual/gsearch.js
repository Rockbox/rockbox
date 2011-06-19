function gsearch() {
    var expr=/(.*)\/([^/]+)$/;
    var loc = expr.exec(window.location)[1];
    document.getElementById("gsearch").innerHTML = ''
    + '<form action="http://www.google.com/search">'
    + 'Search this manual '
    + '<input name="as_q" size="30">'
    + '<input value="Google it" type="submit">'
    + '<input type="hidden" name="as_sitesearch" value="' + loc + '">'
    + '</form>';
}
