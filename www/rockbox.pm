
# short name to image mapping
%model=("player" => "/docs/newplayer_t.jpg",
        "recorder" => "/docs/recorder_t.jpg",
        "fmrecorder" => "/docs/fmrecorder_t.jpg",
        "recorderv2" => "/docs/fmrecorder_t.jpg", 
        "recorder8mb" => "/docs/recorder_t.jpg",
        "fmrecorder8mb" => "/docs/fmrecorder_t.jpg",
        'ondiosp' => "/docs/ondiosp_t.jpg",
        'ondiofm' => "/docs/ondiofm_t.jpg",
        'h100' => "/docs/h100_t.jpg",
        'h120' => "/docs/h100_t.jpg",
        'h300' => "/docs/h300-60x80.jpg",
        'ipodcolor' => "/docs/color_t.jpg",
        'ipodnano' => "/docs/nano_t.jpg",
        'ipod4gray' => "/docs/ipod4g2pp_t.jpg",
        'ipodvideo' => "/docs/ipodvideo_t.jpg",
        'ipod3g' => "/docs/ipod4g2pp_t.jpg",
        'iaudiox5' => "/docs/iaudiox5_t.jpg",
        "install" => "/docs/install.png",
        "source" => "/rockbox100.png");

# short name to long name mapping
%longname=("player" => "Archos Player/Studio",
           "recorder" => "Archos Recorder v1",
           "fmrecorder" => "Archos FM Recorder",
           "recorderv2" => "Archos Recorder v2", 
           "recorder8mb" => "Archos Recorder 8MB",
           "fmrecorder8mb" => "Archos FM Recorder 8MB",
           'ondiosp' => "Archos Ondio SP",
           'ondiofm' => "Archos Ondio FM",
           'h100' => "iriver H100",
           'h120' => "iriver H120",
           'h300' => 'iriver H300',
           'ipodcolor' => 'iPod color/Photo',
           'ipodnano' => 'iPod Nano',
           'ipod4gray' => 'iPod 4G Grayscale',
           'ipodvideo' => 'iPod Video',
           'ipod3g' => 'iPod 3G',
           'iaudiox5' => 'iAudio X5',
           "install" => "Windows Installer",
           "source" => "Source Archive");

sub header {
    my ($t) = @_;
    print "Content-Type: text/html\n\n";
    open (HEAD, "/home/bjst/rockbox_html/head.html");
    while(<HEAD>) {
        $_ =~ s:^<title>Rockbox<\/title>:<title>$t<\/title>:;
        $_ =~ s:^<h1>_PAGE_<\/h1>:<h1>$t<\/h1>:;
        print $_;
    }
    close(HEAD);
}

sub footer {
    open (FOOT, "/home/bjst/rockbox_html/foot.html");
    while(<FOOT>) {
        print $_;
    }
    close(FOOT);
}

1;
