#!/usr/bin/ruby
# (c) 2010 by Thomas Martitz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

#
# parse test codec output files and give wiki or spreadsheet formatted output
#
class CodecResult
    include Comparable
private

    attr_writer :codec
    attr_writer :decoded_frames
    attr_writer :max_frames
    attr_writer :decode_time
    attr_writer :file_duration
    attr_writer :percent_realtime
    attr_writer :mhz_needed

    def get_codec(filename)
        case filename
            when /nero_hev2_.+/
                self.codec = "Nero AAC-HEv2 (SBR+PS)"
            when /.+aache.+/, /nero_he_.+/
                self.codec = "Nero AAC-HE (SBR)"
            when /a52.+/
                self.codec = "AC3 (A52)"
            when /ape_.+/
                self.codec = "Monkey Audio"
            when /lame_.+/
                self.codec = "MP3"
            when /.+\.m4a/
                self.codec = "AAC-LC"
            when /vorbis.+/
                self.codec = "Vorbis"
            when /wma_.+/
                self.codec = "WMA Standard"
            when /wv_.+/
                self.codec = "WAVPACK"
            when /applelossless.+/
                self.codec = "Apple Lossless"
            when /mpc_.+/
                self.codec = "Musepack"
            when /flac_.+/
                self.codec = "FLAC"
            when /cook_.+/
                self.codec = "Cook (RA)"
            when /atrac3.+/
                self.codec = "Atrac3"
            when /true.+/
                self.codec = "True Audio"
            when /toolame.+/, /pegase_l2.+/
                self.codec = "MP2"
            when /atrack1.+/
                self.codec = "Atrac1"
            when /wmapro.+/
                self.codec = "WMA Professional"
            when /wmal.+/
                self.codec = "WMA Lossless"
            when /speex.+/
                self.codec = "Speex"
            when /pegase_l1.+/
                self.codec = "MP1"
            else
                self.codec = "CODEC UNKNOWN (#{name})"
        end
    end

    def file_name=(name)
        @file_name = name
        get_codec(name)
    end

public

    attr_reader :file_name
    attr_reader :codec
    attr_reader :decoded_frames
    attr_reader :max_frames
    attr_reader :decode_time
    attr_reader :file_duration
    attr_reader :percent_realtime
    attr_reader :mhz_needed

    # make results comparable, allows for simple faster/slower/equal
    def <=>(other)
        if self.file_name != other.file_name
            raise ArgumentError, "Cannot compare different files"
        end
        return self.decode_time <=> other.decode_time
    end

    def initialize(text_block, cpu_freq = nil)
        # we need an Array
        c = text_block.class
        if (c != Array && c.superclass != Array)
            raise ArgumentError,
                 "Argument must be an array but is " + text_block.class.to_s
        end

        #~ lame_192.mp3
        #~ 175909 of 175960
        #~ Decode time - 8.84s
        #~ File duration - 175.96s
        #~ 1990.49% realtime
        #~ 30.14MHz needed for realtime (not there in RaaA)

        # file name
        self.file_name = text_block[0]

        # decoded & max frames
        test = Regexp.new(/(\d+) of (\d+)/)
        res = text_block[1].match(test)
        self.decoded_frames = res[1].to_i
        self.max_frames = res[2].to_i

        # decode time, in centiseconds
        test = Regexp.new(/Decode time - ([.\d]+)s/)
        self.decode_time = text_block[2].match(test)[1].to_f

        # file duration, in centiseconds
        test = Regexp.new(/File duration - ([.\d]+)s/)
        self.file_duration = text_block[3].match(test)[1].to_f

        # % realtime
        self.percent_realtime = text_block[4].to_f

        # MHz needed for rt
        test = Regexp.new(/[.\d]+MHz needed for realtime/)
        self.mhz_needed = nil
        if (text_block[5] != nil && text_block[5].length > 0)
            self.mhz_needed = text_block[5].match(test)[0].to_f
        elsif (cpu_freq)
            # if not given, calculate it as per passed cpu frequency
            # duration to microseconds
            speed = self.file_duration / self.decode_time
            self.mhz_needed = cpu_freq / speed
        end
    end
end

class TestCodecResults < Array
    def initialize(file_name, cpu_freq)
        super()
        temp = self.clone
        # go through the results, create a CodecResult for each block
        # of text (results for the codecs are seperated by an empty line)
        File.open(file_name, File::RDONLY) do |file|
            file.each_chomp do |line|
                if (line.length == 0) then
                    self << CodecResult.new(temp, cpu_freq);temp.clear
                else
                    temp << line
                end
            end
        end
    end

    # sort the results by filename (so files of the same codec are near)
    def sort
        super { |x, y| x.file_name <=> y.file_name }
    end
end

class File
    # walk through each line but have the \n removed
    def each_chomp
        self.each_line do |line|
            yield(line.chomp)
        end
    end
end

class Float
    alias_method(:old_to_s, :to_s)
    # add the ability to use a different decimal seperator in to_s
    def to_s
        string = old_to_s
        string.sub!(/[.]/ , @@dec_sep) if @@dec_sep
        string
    end

    @@dec_sep = nil
    def self.decimal_seperator=(sep)
        @@dec_sep=sep
    end
end

#files is an Array of TestCodecResultss
def for_calc(files)
    files[0].each_index do |i|
        string = files[0][i].file_name + "\t"
        for f in files
            string += f[i].percent_realtime.to_s + "%\t"
        end
        puts string
    end
end

#files is an Array of TestCodecResultss
def for_wiki(files)
    basefile = files.shift
    codec = nil
    basefile.each_index do |i| res = basefile[i]
        # make a joined row for each codec
        if (codec == nil || res.codec != codec) then
            codec = res.codec
            puts "| *%s*  ||||%s" % [codec, "|"*files.length]
        end
        row = sprintf("| %s | %.2f%%%% realtime | Decode time - %.2fs |" %
                        [res.file_name, res.percent_realtime, res.decode_time])
        if (res.mhz_needed != nil) # column for mhz needed, | - | if unknown
            row += sprintf(" %.2fMHz |" % res.mhz_needed.to_s)
        else
            row += " - |"
        end
        for f in files  # calculate speed up compared to the rest files
            delta = (res.percent_realtime / f[i].percent_realtime)*100
            row += sprintf(" %.2f%%%% |" % delta)
        end
        puts row
    end
end

# for_xml() anyone? :)

def help
    puts "#{$0} [OPTIONS] FILE [FILES]..."
    puts "Options:\t-w\tOutput in Fosswiki format (default)"
    puts "\t\t-c\tOutput in Spreadsheet-compatible format (tab-seperated)"
    puts "\t\t-s=MHZ\tAssume MHZ cpu frequency for \"MHz needed for realtime\" calculation"
    puts "\t\t\t(if not given by the log files, e.g. for RaaA)"
    puts "\t\t-d=CHAR\tUse CHAR as decimal seperator in the -c output"
    puts "\t\t\t(if your spreadsheed tool localized and making problems)"
    puts
    puts "\tOne file is needed. This is the basefile."
    puts "\tIn -c output, the % realtime values of each"
    puts "\tcodec from each file is printed on the screen onto the screen"
    puts "\tIn -w output, a wiki table is made from the basefile with one column"
    puts "\tfor each additional file representing relative speed of the basefile"
    exit
end

to_call = method(:for_wiki)
mhz = nil
files = []

help if (ARGV.length == 0)

ARGV.each do |e|
    a = e.chars.to_a
    if (a[0] == '-') # option
        case a[1]
            when 'c'
                to_call = method(:for_calc)
            when 'w'
                to_call = method(:for_wiki)
            when 'd'
                if (a[2] == '=')
                    sep = a[3]
                else
                    sep = a[2]
                end
                Float.decimal_seperator = sep
            when 's'
                if (a[2] == '=')
                    mhz = a[3..-1].join.to_i
                else
                    mhz = a[2..-1].join.to_i
                end
            else
                help
        end
    else # filename
        files << e
    end
end


tmp = []
for file in files do
    tmp << TestCodecResults.new(file, mhz).sort
end
to_call.call(tmp) # invoke selected method
