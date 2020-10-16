# Note!  This is used by the www.rockbox.org infrastructure
# to know what targets/etc should be displayed, built, and so forth.
#
$publicrelease="3.15";
$releasedate="15 Nov 2019";
$releasenotes="/wiki/ReleaseNotes315";

################################################################

# 'modelname' => {
#    name => 'Full Name',
#    status => 1,                        # 0=retired, 1=unusable, 2=unstable, 3=stable
#    ram => 2,                           # optional (used?)
#    manual => 'modelname2',             # optional (uses modelname2's manual)
#    icon => 'modelname3',               # optional (uses modelname3's icon)
#    voice => 'modelname4'               # optional (uses modelname4's voice)
#    release => '3.14',                  # optional (final release version, if different from above)
# }

%builds = (
    'archosfmrecorder' => {
        name => 'Archos FM Recorder',
        status => 0,
        ram => 2,
        release => '3.15', 
    },
    'archosondiofm' => {
        name => 'Archos Ondio FM',
        status => 0,
        ram => 2,
        release => '3.15', 
    },
    'archosondiosp' => {
        name => 'Archos Ondio SP',
        status => 0,
        ram => 2,
        release => '3.15', 
    },
    'archosplayer' => {
        name => 'Archos Player/Studio',
        status => 0,
        ram => 2,
        release => '3.15', 
    },
    'archosrecorder' => {
        name => 'Archos Recorder v1',
        status => 0,  # Has not compiled since 2013
        ram => 2,
        release => '3.13',  # Haven't had a proper release since then.
    },
    'archosrecorderv2' => {
        name => 'Archos Recorder v2',
        status => 0,
        ram => 2,
        manual => "archosfmrecorder",
        release => '3.15', 
    },
    'cowond2' => {
        name => 'Cowon D2',
        status => 2,
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
        status => 3,
    },
    'gogearhdd6330' => {
        name => 'Philips GoGear HDD6330',
        status => 3,
    },
    'gogearsa9200' => {
        name => 'Philips GoGear SA9200',
        status => 3,
    },
    'hifietma9' => {
        name => 'HiFi E.T MA9',
        status => 2,
    },
    'hifietma9c' => {
        name => 'HiFi E.T MA9C',
        status => 2,
    },
    'hifietma8' => {
        name => 'HiFi E.T MA8',
        status => 2,
    },
    'hifietma8c' => {
        name => 'HiFi E.T MA8C',
        status => 2,
    },
    'hifimanhm60x' => {
        name => 'HiFiMAN HM-60x',
        status => 2,
    },
    'hifimanhm801' => {
        name => 'HiFiMAN HM-801',
        status => 2,
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
    'ibassodx50' => {
        name => 'iBasso DX50',
        status => 2,
    },
    'ibassodx90' => {
        name => 'iBasso DX90',
        status => 2,
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
        status => 3,
    },
    'ipodvideo' => {
        name => 'iPod Video',
        status => 3,
    },
    'ipod6g' => {
        name => 'iPod 6th gen (Classic)',
        status => 3,
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
    'rk27generic' => {
        name => 'Rockchip rk27xx',
        status => 1,
    },
    'samsungyh820' => {
        name => 'Samsung YH-820',
        status => 3,
    },
    'samsungyh920' => {
        name => 'Samsung YH-920',
        status => 3,
    },
    'samsungyh925' => {
        name => 'Samsung YH-925',
        status => 3,
    },
    'samsungypr0' => {
        name => 'Samsung YP-R0',
        status => 2,
    },
    'samsungypr1' => {
        name => 'Samsung YP-R1',
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
        status => 3,
        icon => 'sansac200',
    },
    'sansaclip' => {
        name => 'SanDisk Sansa Clip v1',
        status => 3,
    },
    'sansaclipv2' => {
        name => 'SanDisk Sansa Clip v2',
        status => 3,
        icon => 'sansaclip',
    },
    'sansaclipplus' => {
        name => 'SanDisk Sansa Clip+',
        status => 3,
    },
    'sansaclipzip' => {
        name => 'SanDisk Sansa Clip Zip',
        status => 3,
    },
    'sansae200' => {
        name => 'SanDisk Sansa e200',
        status => 3,
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
    'sansafuzev2' => {
        name => 'SanDisk Sansa Fuze v2',
        status => 3,
        icon => 'sansafuze',
    },
    'sansafuzeplus' => {
        name => 'SanDisk Sansa Fuze+',
        status => 3,
        icon => 'sansafuzeplus',
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
        status => 3,
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
    'mpiohd200' => {
        name => 'MPIO HD200',
        status => 2,
    },
    'mpiohd300' => {
        name => 'MPIO HD300',
        status => 3,
    },
    'creativezenxfi2' => {
        name => 'Creative Zen X-Fi2',
        status => 2,
    },
    'creativezenxfi3' => {
        name => 'Creative Zen X-Fi3',
        status => 3,
    },
    'sonynwze350' => {
        name => 'Sony NWZ-E350',
        status => 2,
    },
    'sonynwze360' => {
        name => 'Sony NWZ-E360',
        status => 3,
    },
    'sonynwze370' => {
        name => 'Sony NWZ-E370/E380',
        status => 3,
    },
    'sonynwze450' => {
        name => 'Sony NWZ-E450',
        status => 2,
    },
    'sonynwze460' => {
        name => 'Sony NWZ-E460',
        status => 2,
    },
    'sonynwze470' => {
        name => 'Sony NWZ-E470',
        status => 2,
    },
    'sonynwze580' => {
        name => 'Sony NWZ-E580',
        status => 2,
    },
    'sonynwza10' => {
        name => 'Sony NWZ-A10',
        status => 2,
    },
    'sonynwa20' => {
        name => 'Sony NW-A20',
        status => 2,
    },
    'sonynwza860' => {
        name => 'Sony NWZ-A860',
        status => 2,
    },
    'sonynwzs750' => {
        name => 'Sony NWZ-S750',
        status => 2,
    },
    'creativezenxfi' => {
        name => 'Creative Zen X-Fi',
        status => 3
    },
    'creativezenxfistyle' => {
        name => 'Creative Zen X-Fi Style',
        status => 3
    },
    'creativezen' => {
        name => 'Creative Zen',
        status => 2
    },
    'creativezenmozaic' => {
        name => 'Creative Zen Mozaic',
        status => 3
    },
    'agptekrocker' => {
        name => 'Agptek Rocker',
        status => 2
    },
    'xduoox3' => {
        name => 'xDuoo X3',
        status => 3,
    },
    'xduoox3ii' => {
        name => 'xDuoo X3ii',
        status => 2,
    },
    'xduoox20' => {
        name => 'xDuoo X20',
        status => 2,
    },
    'fiiom3k' => {
        name => 'FiiO M3K',
        status => 1,
    },
    'aigoerosq' => {
        name => 'AIGO EROS Q / K',
        status => 2,
    },
    'ihifi770' => {
        name => 'Xuelin iHIFI 770',
        status => 2,
    },
    'ihifi770c' => {
        name => 'Xuelin iHIFI 770C',
        status => 2,
    },
    'ihifi800' => {
        name => 'Xuelin iHIFI 800',
        status => 2,
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
        push @list, $b if ($builds{$b}{status} >= 3) or $builds{$b}{release};
    }

    return @list;
}

sub allbuilds {
    my @list;

    for my $b (sort byname keys %builds) {
        push @list, $b;
    }

    return @list;
}

################################################################

# 'voicename' => {
#    lang => 'langname',                     # source rockbox .lang file
#    name => 'Native Name ( English Name )', # descriptive text
#    short => 'xx',                          # short iso621-ish text
#    defengine => 'enginename',              # which engine to prefer
#    engines => {                            # supported engines
#      enginenamea = '-opt1=x -opt2=y',        # options for enginea
#      enginenameb = '-lang=xx',               # options for engineb
#    },
#    enabled => 1,                           # set to 0 or leave out to disable
# }

# A single source language file can have many voice variants.
# For example, Mandarin and Cantonese use the same "Chinese" script.
# Also, different genders or regional accents for the same language

%voices = (
    # UK English always comes first; it's the "master"
    'english' => {
	'lang' => 'english',
	'name' => 'UK English',
	'short' => 'en-gb',
        'defengine' => 'espeak',
	'engines' => {
	    'festival' => '--language english',
	    'espeak' => '-ven-gb -k 5',
	    'gtts' => '-l en-gb',
	},
        'enabled' => 1,
    },
    # Everything else in alphabetical order
    'deutsch' => {
	'lang' => 'deutsch',
	'name' => 'Deutsch (German)',
        'short' => 'de',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vde',
	    'gtts' => '-l de',
	},
    },
    'english-us' => {
	'lang' => 'english-us',
	'name' => 'American English',
        'short' => 'en-us',
        'defengine' => 'espeak',
	'engines' => {
	    'festival' => '--language english',
	    'espeak' => '-ven-us -k 5',
	    'gtts' => '-l en-us',
	},
        'enabled' => 1,
    },
    'francais' => {
	'lang' => 'francais',
	'name' => 'Français (French)',
        'short' => 'fr',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vfr-fr',
	    'gtts' => '-l fr',
	},
        'enabled' => 1,
    },
    'greek' => {
	'lang' => 'greek',
	'name' => 'Ελληνικά (Greek)',
        'short' => 'el',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vel',
	    'gtts' => '-l el',
	},
        'enabled' => 1,
    },
    'italiano' => {
	'lang' => 'italiano',
	'name' => 'Italiano (Italian)',
        'short' => 'it',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vit',
	    'gtts' => '-l it',
	},
        'enabled' => 1,
    },
    'norsk' => {
	'lang' => 'norsk',
	'name' => 'Norsk (Norwegian)',
        'short' => 'no',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vno',
	    'gtts' => '-l no',
	},
    },
    'polski' => {
	'lang' => 'polski',
	'name' => 'Polski (Polish)',
        'short' => 'pl',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vpl',
	    'gtts' => '-l pl',
	},
        'enabled' => 1,
    },
    'russian' => {
	'lang' => 'russian',
	'name' => 'Русский (Russian)',
        'short' => 'ru',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vru',
	    'gtts' => '-l ru',
	},
    },
    'slovak' => {
	'lang' => 'slovak',
	'name' => 'Slovenský (Slovak)',
        'short' => 'sk',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vsk',
	    'gtts' => '-l sk',
	},
        'enabled' => 1,
    },
    'srpski' => {
	'lang' => 'srpski',
	'name' => 'српски (Serbian)',
        'short' => 'sr',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vsr',
	    'gtts' => '-l sr',
	},
    },
);

sub bylang {
    return uc $voices{$a}{lang} cmp uc $voices{$b}{lang};
}

sub allvoices {
    my @list;

    for my $b (sort bylang keys %voices) {
        push @list, $b if (defined($voices{$b}->{enabled}) && $voices{$b}->{enabled});
    }

    return @list;
}

sub voicesforlang($) {
    my $l = shift @_;
    my @list;

    for my $b (sort bylang keys %voices) {
        push @list, $b if ($voices{$b}{lang} eq $b && defined($voices{$b}->{enabled}) && $voices{$b}->{enabled});
    }

    return @list;
}

1;
