# Note!  This is used by the www.rockbox.org infrastructure
# to know what targets/etc should be displayed, built, and so forth.
#
$publicrelease="4.0";
$releasedate="1 Apr 2025";
$releasenotes="/wiki/ReleaseNotes400";

################################################################

# 'modelname' => {
#    name => 'Full Name',
#    status => 1,                        # 0=retired, 1=unusable, 2=unstable, 3=stable
#    sim = 1,                            # optional (defaults 1 for status 2/3 and 0 for status 1)
#    ram => 2,                           # optional (used?)
#    manual => 'modelname2',             # optional (uses modelname2's manual)
#    icon => 'modelname3',               # optional (uses modelname3's icon)
#    voice => 'modelname4'               # optional (uses modelname4's voice)
#    release => '3.14',                  # optional (final release version, if different from above. can be in future)
#    manualok => 1,                      # optional (defaults 1 for status 3 and 0 for rest)
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
        manualok => 0,
        status => 3,
    },
    'gogearhdd6330' => {
        name => 'Philips GoGear HDD6330',
        manualok => 0,
        status => 3,
    },
    'gogearsa9200' => {
        name => 'Philips GoGear SA9200',
        status => 3,
    },
    'hifietma9' => {
        name => 'HiFi E.T MA9',
	sim => 0,
        status => 2,
    },
    'hifietma9c' => {
        name => 'HiFi E.T MA9C',
	sim => 0,
        status => 2,
    },
    'hifietma8' => {
        name => 'HiFi E.T MA8',
        status => 2,
	sim => 0,
    },
    'hifietma8c' => {
        name => 'HiFi E.T MA8C',
        status => 2,
	sim => 0,
    },
    'hifimanhm60x' => {
        name => 'HiFiMAN HM-60x',
	sim => 0,
        status => 2,
    },
    'hifimanhm801' => {
        name => 'HiFiMAN HM-801',
	sim => 0,
        status => 2,
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
	sim => 0,
        status => 2,
    },
    'ibassodx90' => {
        name => 'iBasso DX90',
	sim => 0,
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
	sim => 1,
    },
    'ondavx747p' => {
        name => 'Onda VX747+',
        status => 1,
	sim => 1,
    },
    'ondavx767' => {
        name => 'Onda VX767',
        status => 1,
    },
    'ondavx777' => {
        name => 'Onda VX777',
        status => 1,
	sim => 1,
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
	sim => 0,
        status => 2,
    },
    'samsungyps3' => {
        name => 'Samsung YP-S3',
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
    'sansaconnect' => {
        name => 'SanDisk Sansa Connect',
        status => 2,
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
    'sansam200v4' => {
        name => 'SanDisk Sansa m200 v4',
        status => 1,
	sim => 1,
    },
    'sansaview' => {
        name => 'SanDisk Sansa View',
        status => 1,
    },
    'vibe500' => {
        name => 'Packard Bell Vibe 500',
        status => 3,
    },
    'zenvision' => {
        name => 'Creative Zen Vision',
        status => 1,
	sim => 1,
    },
    'zenvisionm30gb' => {
        name => 'Creative Zen Vision:M 30GB',
        status => 1,
	sim => 1,
    },
    'zenvisionm60gb' => {
        name => 'Creative Zen Vision:M 60GB',
        status => 1,
	sim => 1,
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
        manualok => 0,
    },
    'sonynwze350' => {
        name => 'Sony NWZ-E350',
	sim => 0,
        status => 2,
    },
    'sonynwze360' => {
        name => 'Sony NWZ-E360',
        status => 3,
        manualok => 0,
    },
    'sonynwze370' => {
        name => 'Sony NWZ-E370/E380',
        status => 3,
        manualok => 0,
    },
    'sonynwze450' => {
        name => 'Sony NWZ-E450',
	sim => 0,
        status => 2,
    },
    'sonynwze460' => {
        name => 'Sony NWZ-E460',
	sim => 0,
        status => 2,
    },
    'sonynwze470' => {
        name => 'Sony NWZ-E470',
	sim => 0,
        status => 2,
    },
    'sonynwze580' => {
        name => 'Sony NWZ-E580',
	sim => 0,
        status => 2,
    },
    'sonynwza10' => {
        name => 'Sony NWZ-A10',
	sim => 0,
        status => 2,
    },
    'sonynwa20' => {
        name => 'Sony NW-A20',
	sim => 0,
        status => 2,
    },
    'sonynwza860' => {
        name => 'Sony NWZ-A860',
        status => 2,
    },
    'sonynwzs750' => {
        name => 'Sony NWZ-S750',
        status => 2,
	sim => 0,
    },
    'creativezenxfi' => {
        name => 'Creative Zen X-Fi',
        status => 3,
        manualok => 0,
    },
    'creativezenxfistyle' => {
        name => 'Creative Zen X-Fi Style',
        status => 3,
        manualok => 0,
    },
    'creativezen' => {
        name => 'Creative Zen',
        status => 2,
    },
    'creativezenv' => {
        name => 'Creative Zen V',
        status => 1,
    },
    'creativezenmozaic' => {
        name => 'Creative Zen Mozaic',
        status => 3,
        manualok => 0,
    },
    'agptekrocker' => {
        name => 'Agptek Rocker',
        status => 3,
    },
    'xduoox3' => {
        name => 'xDuoo X3',
        status => 3,
    },
    'xduoox3ii' => {
        name => 'xDuoo X3ii',
        manualok => 0,     # Remove when manual is written
        status => 3,
    },
    'xduoox20' => {
        name => 'xDuoo X20',
        manualok => 0,     # Remove when manual is written
        status => 3,
    },
    'fiiom3klinux' => {
        name => 'FiiO M3K (Linux)',
        status => 1,
    },
    'fiiom3k' => {
        name => 'FiiO M3K',
        status => 3,
    },
    'aigoerosq' => {
        name => 'AIGO EROS Q / K (Hosted)',
        status => 2,  # Do we promote this to stable?
        # manual => "erosqnative",
    },
    'erosqnative' => {
        name => 'AIGO EROS Q / K (Native)',
        status => 3,
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
    'shanlingq1' => {
        name => 'Shanling Q1',
        status => 3,
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

sub manualbuilds {
    my @list;

    for my $b (sort byname keys %builds) {
        push @list, $b if (($builds{$b}{status} > 2 && !defined($builds{$b}{manualok})) ||
                          (defined($builds{$b}{manualok}) && ($builds{$b}{manualok} > 0)));
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

sub simbuilds {
    my @list;

    for my $b (&allbuilds) {
        push @list, $b if (defined($builds{$b}{sim}) and $builds{$b}{sim});
        push @list, $b if (!defined($builds{$b}{sim}) and $builds{$b}{status} > 1);
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
        'defengine' => 'piper',
	'engines' => {
	    'festival' => '--language english',
	    'espeak' => '-ven-gb -k 5',
	    'gtts' => '-l en -t co.uk',
	    'piper' => 'en_GB-semaine-medium.onnx',
	},
        'enabled' => 1,
    },
    # Everything else in alphabetical order
    'bulgarian' => {
	'lang' => 'bulgarian',
	'name' => 'Български (Bulgarian)',
        'short' => 'bg',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vbg',
	    'gtts' => '-l bg',
            # No piper voice yet.
	},
        'enabled' => 1,
    },
    'chinese-simp' => {  # Mandarin?
	'lang' => 'chinese-simp',
	'name' => '简体中文 (Chinese Simplified)',
        'short' => 'zh-cn',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vzh',
	    'gtts' => '-l zh',
	    'piper' => 'zh_CN-huayan-medium.onnx',
	},
        'enabled' => 1,
    },
    'czech' => {
	'lang' => 'czech',
	'name' => 'Čeština (Czech)',
        'short' => 'cs',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vcs',
	    'gtts' => '-l cs',
	    'piper' => 'cs_CZ-jirka-medium.onnx',
	},
        'enabled' => 0,
    },
    'dansk' => {
	'lang' => 'dansk',
	'name' => 'Dansk (Danish)',
        'short' => 'da',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vda',
	    'gtts' => '-l da',
            'piper' => 'da_DK-talesyntese-medium.onnx',
	},
        'enabled' => 0,
    },
    'deutsch' => {
	'lang' => 'deutsch',
	'name' => 'Deutsch (German)',
        'short' => 'de',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vde',
	    'gtts' => '-l de',
            'piper' => 'de_DE-thorsten-high.onnx',
	},
        'enabled' => 1,
    },
    'eesti' => {
	'lang' => 'eesti',
	'name' => 'Eesti (Estonian)',
        'short' => 'et',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vet',
	    'gtts' => '-l et',
            # No piper voice yet.
	},
        'enabled' => 0,
    },
    'english-us' => {
	'lang' => 'english-us',
	'name' => 'American English',
        'short' => 'en-us',
        'defengine' => 'piper',
	'engines' => {
	    'festival' => '--language english',
	    'espeak' => '-ven-us -k 5',
	    'gtts' => '-l en -t us',
            'piper' => 'en_US-lessac-high.onnx',
	},
        'enabled' => 1,
    },
    'espanol' => {
	'lang' => 'espanol',
	'name' => 'Spanish (Peninsular)',
        'short' => 'es-es',
        'defengine' => 'piper',
	'engines' => {
	    'festival' => '--language spanish',
	    'espeak' => '-ves -k 5',
	    'gtts' => '-l es',
            'piper' => 'es_ES-sharvard-medium.onnx',
	},
        'enabled' => 1,
    },
    'espanol-mx' => {
	'lang' => 'espanol',
	'name' => 'Spanish (Mexican)',
        'short' => 'es-mx',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-ves -k 6',
            'gtts' => '-l es -t mx',
            'piper' => 'es_MX-claude-high.onnx',
	},
        'enabled' => 1,
    },
    'francais' => {
	'lang' => 'francais',
	'name' => 'Français (French)',
        'short' => 'fr',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vfr-fr',
	    'gtts' => '-l fr',
            'piper' => 'fr_FR-siwis-medium.onnx',
	},
        'enabled' => 1,
    },
    'greek' => {
	'lang' => 'greek',
	'name' => 'Ελληνικά (Greek)',
        'short' => 'el',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vel',
	    'gtts' => '-l el',
            'piper' => 'el_GR-rapunzelina-low.onnx',
	},
        'enabled' => 0,
    },
    'italiano' => {
	'lang' => 'italiano',
	'name' => 'Italiano (Italian)',
        'short' => 'it',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vit',
	    'gtts' => '-l it',
            'piper' => 'it_IT-paola-medium.onnx',
	},
        'enabled' => 1,
    },
    'japanese' => {
	'lang' => 'japanese',
	'name' => '日本語 (Japanese)',
        'short' => 'ja',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vja',
	    'gtts' => '-l ja',
	},
        'enabled' => 1,
    },
    'korean' => {
	'lang' => 'korean',
	'name' => '한국어 (Korean)',
        'short' => 'ko',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vko',
	    'gtts' => '-l ko',
	},
        'enabled' => 1,
    },
    'latviesu' => {
	'lang' => 'latviesu',
	'name' => 'Latviešu (Latvian)',
        'short' => 'lv',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vlv',
	    'gtts' => '-l lv',
            'piper' => 'lv_LV-aivars-medium.onnx',
	},
	'enabled' => 1,
    },
    'nederlands' => {
	'lang' => 'nederlands',
	'name' => 'Nederlands (Dutch)',
        'short' => 'nl',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vnl',
	    'gtts' => '-l nl',
            'piper' => 'nl_NL-mls-medium.onnx',
	},
	'enabled' => 1,
    },
    'norsk' => {
	'lang' => 'norsk',
	'name' => 'Norsk (Norwegian)',
        'short' => 'no',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vno',
	    'gtts' => '-l no',
            'piper' => 'no_NO-talesyntese-medium.onnx',
	},
        'enabled' => 0,
    },
    'moldoveneste' => {
	'lang' => 'moldoveneste',
	'name' => 'Moldovenească (Moldavian)',
        'short' => 'ro-md',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vro',
	    'gtts' => '-t md -l ro',
            'piper' => 'ro_RO-mihai-medium.onnx',
	},
        'enabled' => 1,
    },
    'polski' => {
	'lang' => 'polski',
	'name' => 'Polski (Polish)',
        'short' => 'pl',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vpl',
	    'gtts' => '-l pl',
            'piper' => 'pl_PL-gosia-medium.onnx',
	},
        'enabled' => 1,
    },
    'portugues-brasileiro' => {
	'lang' => 'portugues-brasileiro',
	'name' => 'Portuguese (Brazilian)',
        'short' => 'pt-br',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vpt-br',
	    'gtts' => '-l pt -t br',
            'piper' => 'pt_BR-faber-medium.onnx',
	},
        'enabled' => 1,
    },
    'romaneste' => {
	'lang' => 'romaneste',
	'name' => 'Română (Romanian)',
        'short' => 'ro',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vro',
	    'gtts' => '-l ro',
            'piper' => 'ro_RO-mihai-medium.onnx',
	},
        'enabled' => 1,
    },
    'russian' => {
	'lang' => 'russian',
	'name' => 'Русский (Russian)',
        'short' => 'ru',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vru',
	    'gtts' => '-l ru',
            'piper' => 'ru_RU-irina-medium.onnx',
	},
        'enabled' => 1,
    },
    'slovak' => {
	'lang' => 'slovak',
	'name' => 'Slovenský (Slovak)',
        'short' => 'sk',
        'defengine' => 'espeak',
	'engines' => {
	    'espeak' => '-vsk',
	    'gtts' => '-l sk',
            'piper' => 'sk_SK-lili-medium.onnx',
	},
        'enabled' => 1,
    },
    'srpski' => {
	'lang' => 'srpski',
	'name' => 'српски (Serbian)',
        'short' => 'sr',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vsr',
	    'gtts' => '-l sr',
            'piper' => 'sr_RS-serbski_institut-medium.onnx',
	},
        'enabled' => 1,
    },
    'svenska' => {
	'lang' => 'svenska',
	'name' => 'Svenska (Swedish)',
        'short' => 'sv',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vsv',
	    'gtts' => '-l sv',
            'piper' => 'sv_SE-nst-medium.onnx',
	},
        'enabled' => 1,
    },
    'turkce' => {
	'lang' => 'turkce',
	'name' => 'Türkçe (Turkish)',
        'short' => 'tr',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vtr',
	    'gtts' => '-l tr',
            'piper' => 'tr_TR-fettah-medium.onnx',
	},
        'enabled' => 1,
    },
    'ukrainian' => {
	'lang' => 'ukrainian',
	'name' => 'украї́нська (Ukrainian)',
        'short' => 'uk',
        'defengine' => 'piper',
	'engines' => {
	    'espeak' => '-vuk',
	    'gtts' => '-l uk',
            'piper' => 'uk_UA-ukrainian_tts-medium.onnx',
	},
        'enabled' => 1,
    },
);

sub byshortname {
    return uc $voices{$a}{short} cmp uc $voices{$b}{short};
}

sub allvoices {
    my @list;

    for my $b (sort byshortname keys %voices) {
        push @list, $b if (defined($voices{$b}->{enabled}) && $voices{$b}->{enabled});
    }

    return @list;
}

sub voicesforlang($) {
    my $l = shift @_;
    my @list;

    for my $b (sort byshortname keys %voices) {
        push @list, $b if ($voices{$b}{lang} eq $b && defined($voices{$b}->{enabled}) && $voices{$b}->{enabled});
    }

    return @list;
}

1;
