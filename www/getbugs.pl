#!/usr/bin/perl

@html = `curl --silent "http://sourceforge.net/tracker/?group_id=44306&atid=439118"`;

@entries = grep {/HREF=\"\/tracker\/index.php/} @html;

print "<table class=bugs>\n";
print "<tr><th>submitted</th><th>id</th><th>summary</th><th>submitted by</th><th>assigned to</th></tr>\n";
for ( @entries ) {
    if ( /NOWRAP>(\d+).*?HREF=\"(.*?)\">(.*?)<.*?(nbsp;|\*) (.*?)<.*?\>(\w+)<.*?\>(\w+)</ ) {
        ($submit, $assigned, $date, $id, $num, $summary) = ($7, $6, $5, $2, $1, $3);
        $submit = "<a href=http://sourceforge.net/users/$submit>$submit</a>" if ( $submit ne "nobody" );
        $assigned = "<a href=http://sourceforge.net/users/$assigned>$assigned</a>" if ( $assigned ne "nobody" );
            
        print "<tr><td>$date</td><td><a href=\"http://www.sourceforge.net$id\">$num</a></td><td>$summary</td><td>$submit</td><td>$assigned</td></tr>\n";
    }
}
print "</table>\n";
