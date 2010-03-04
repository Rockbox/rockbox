$publicrelease="3.51";
$releasedate="March 4, 2010";
$releasenotes="/wiki/ReleaseNotes351";

%builds = (
    'archosav300' => {
        name => 'Archos AV300',
        status => 1, # 3=stable, 2=unstable, 1=unusable
    },
    'archosfmrecorder' => {
        name => 'Archos FM Recorder',
        status => 3,
        ram => 2,
    },
    'archosondiofm' => {
        name => 'Archos Ondio FM',
        status => 3,
        ram => 2,
    },
    'archosondiosp' => {
        name => 'Archos Ondio SP',
        status => 3,
        ram => 2,
    },
    'archosplayer' => {
        name => 'Archos Player/Studio',
        status => 3,
        ram => 2,
    },
    'archosrecorder' => {
        name => 'Archos Recorder v1',
        status => 3,
        ram => 2,
    },
    'archosrecorderv2' => {
        name => 'Archos Recorder v2', 
        status => 3,
        ram => 2,
        manual => "archosfmrecorder",
    },
    'cowond2' => {
        name => 'Cowon D2',
        status => 2,
        manual => 'cowond2',
    },
    'gigabeatfx' => {
        name => 'Toshiba Gigabeat F/X',
        status => 3,
    },
    'gigabeats' => {
        name => 'Toshiba Gigabeat S',
        status => 2,
    },
    'gogearhdd1630' => {
        name => 'Philips GoGear HDD1630',
        status => 1,
    },
    'gogearsa9200' => {
        name => 'Philips GoGear SA9200',
        status => 1,
    },
    'iaudio7' => {
        name => 'iAudio 7',
        status => 1,
    },
    'iaudiom3' => {
        name => 'iAudio M3',
        status => 3,
    },
    'iaudiom5' => {
        name => 'iAudio M5',
        status => 3,
    },
    'iaudiox5' => {
        name => 'iAudio X5',
        status => 3,
    },
    'ipod1g2g' => {
        name => 'iPod 1st and 2nd gen',
        status => 3,
    },
    'ipod3g' => {
        name => 'iPod 3rd gen',
        status => 3,
    },
    'ipod4g' => {
        name => 'iPod 4th gen Grayscale',
        status => 3,
    },
    'ipodcolor' => {
        name => 'iPod color/Photo',
        status => 3,
    },
    'ipodmini1g' => {
        name => 'iPod Mini 1st gen',
        status => 3,
    },
    'ipodmini2g' => {
        name => 'iPod Mini 2nd gen',
        status => 3,
        icon => 'ipodmini1g',
        manual => 'ipodmini1g',
    },
    'ipodnano1g' => {
        name => 'iPod Nano 1st gen',
        status => 3,
    },
    'ipodnano2g' => {
        name => 'iPod Nano 2nd gen',
        status => 2,
    },
    'ipodvideo' => {
        name => 'iPod Video 30GB',
        status => 3,
        ram => 32,
    },
    'ipodvideo64mb' => {
        name => 'iPod Video 60/80GB',
        status => 3,
        icon => 'ipodvideo',
        manual => 'ipodvideo',
        voice => 'ipodvideo',
        configname => 'ipodvideo',
        ram => 64,
    },
    'iriverh10' => {
        name => 'iriver H10 20GB',
        status => 3,
    },
    'iriverh10_5gb' => {
        name => 'iriver H10 5GB',
        status => 3,
    },
    'iriverh100' => {
        name => 'iriver H100/115',
        status => 3,
    },
    'iriverh120' => {
        name => 'iriver H120/140',
        status => 3,
        icon => 'iriverh100',
        manual => 'iriverh100',
    },
    'iriverh300' => {
        name => 'iriver H320/340',
        status => 3,
    },
    'iriverifp7xx' => {
        name => 'iriver iFP-7xx',
        status => 1,
    },
    'logikdax' => {
        name => 'Logik DAX',
        status => 1,
    },
    'lyreproto1' => {
        name => 'Lyre Prototype 1',
        status => 1,
    },
    'mini2440' => {
        name => 'Mini 2440',
        status => 1,
    },
    'meizum3' => {
        name => 'Meizu M3',
        status => 1,
    },
    'meizum6sl' => {
        name => 'Meizu M6SL',
        status => 1,
    },
    'meizum6sp' => {
        name => 'Meizu M6SP',
        status => 1,
    },
    'mrobe100' => {
        name => 'Olympus M-Robe 100',
        status => 3,
    },
    'mrobe500' => {
        name => 'Olympus M-Robe 500',
        status => 2,
    },
    'ondavx747' => {
        name => 'Onda VX747',
        status => 1,
    },
    'ondavx747p' => {
        name => 'Onda VX747+',
        status => 1,
    },
    'ondavx767' => {
        name => 'Onda VX767',
        status => 1,
    },
    'ondavx777' => {
        name => 'Onda VX777',
        status => 1,
    },
    'samsungyh820' => {
        name => 'Samsung YH-820',
        status => 2,
    },
    'samsungyh920' => {
        name => 'Samsung YH-920',
        status => 2,
    },
    'samsungyh925' => {
        name => 'Samsung YH-925',
        status => 2,
    },
    'samsungyps3' => {
        name => 'Samsung YP-S3',
        status => 1,
    },
    'sansac100' => {
        name => 'SanDisk Sansa c100',
        status => 1,
    },
    'sansac200' => {
        name => 'SanDisk Sansa c200',
        status => 3,
    },
    'sansac200v2' => {
        name => 'SanDisk Sansa c200 v2',
        status => 1,
        icon => 'sansac200',
    },
    'sansaclip' => {
        name => 'SanDisk Sansa Clip',
        status => 2,
    },
    'sansaclipv2' => {
        name => 'SanDisk Sansa Clip v2',
        status => 1,
    },
    'sansae200' => {
        name => 'SanDisk Sansa e200',
        status => 3,
    },
    'sansae200r' => {
        name => 'SanDisk Sansa e200R',
        status => 3,
        icon => 'sansae200',
        manual => 'sansae200',
    },
    'sansae200v2' => {
        name => 'SanDisk Sansa e200 v2',
        status => 3,
        icon => 'sansae200',
    },
    'sansafuze' => {
        name => 'SanDisk Sansa Fuze',
        status => 3,
    },
    'sansam200' => {
        name => 'SanDisk Sansa m200',
        status => 1,
    },
    'sansam200v4' => {
        name => 'SanDisk Sansa m200 v4',
        status => 1,
    },
    'sansaview' => {
        name => 'SanDisk Sansa View',
        status => 1,
    },
    'tatungtpj1022' => {
        name => 'Tatung Elio TPJ-1022',
        status => 1,
    },
    'vibe500' => {
        name => 'Packard Bell Vibe 500',
        status => 2,
    },
    'zenvision' => {
        name => 'Creative Zen Vision',
        status => 1,
    },
    'zenvisionm30gb' => {
        name => 'Creative Zen Vision:M 30GB',
        status => 1,
    },
    'zenvisionm60gb' => {
        name => 'Creative Zen Vision:M 60GB',
        status => 1,
    },
);

sub manualname {
    my $m = shift @_;

    return $builds{$m}{manual} ? "$builds{$m}{manual}" : $m;
}    

sub voicename {
    my $m = shift @_;

    return $builds{$m}{voice} ? "$builds{$m}{voice}" : $m;
}

sub byname {
    return uc $builds{$a}{name} cmp uc $builds{$b}{name};
}

sub usablebuilds {
    my @list;

    for my $b (sort byname keys %builds) {
        push @list, $b if ($builds{$b}{status} >= 2);
    }

    return @list;
}

sub stablebuilds {
    my @list;

    for my $b (sort byname keys %builds) {
        push @list, $b if ($builds{$b}{status} >= 3);
    }

    return @list;
}

1;
