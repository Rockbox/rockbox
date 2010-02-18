"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
"             __________               __   ___.
"   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
"   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
"   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
"   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
"                     \/            \/     \/    \/            \/
" $Id$
"
" Vim syntax file for Rockbox WPS (While Playing Screen) definitions.
" Copyright (C) 2009 by Kevin Schoedel
"
" This program is free software; you can redistribute it and/or
" modify it under the terms of the GNU General Public License
" as published by the Free Software Foundation; either version 2
" of the License, or (at your option) any later version.
"
" This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
" KIND, either express or implied.
"
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

if exists("b:current_syntax")
    finish
endif

syn case match

" Highlight trailing white space.
if exists("c_space_errors")
    if !exists("c_no_trail_space_error")
        syn match wpsSpaceError display excludenl   "\s\+$"
    endif
    if !exists("c_no_tab_space_error")
        syn match wpsSpaceError display             " \+\t"me=e-1
    endif
endif


" Comments.
syn keyword wpsTodo             contained TODO FIXME XXX
syn cluster wpsCommentGroup     contains=wpsTodo
syn region  wpsComment          start="#" end="$" keepend contains=@wpsCommentGroup,wpsSpaceError,@Spell

" Delimiters.
syn match   wpsPipeError        "|"
syn match   wpsSep              "|"                     contained
syn match   wpsSubline          ";"
syn match   wpsPct              "%"                     nextgroup=wpsSpecial,wpsCond,@wpsTag

" Literals.
syn match   wpsSpecial          "[%<|>;#]"              contained

" Conditional test.
syn match   wpsCond             "?"                     nextgroup=@wpsTag
syn match   wpsElse             "|"                     contained
syn match   wpsEndIfError       ">"
syn match   wpsIfEndIf          "[<>]"                  contained
syn region  wpsConditions       matchgroup=wpsIfEndIf start="<" end=">" skip="%>" contains=wpsElse,wpsConditions,wpsPct,wpsSubline
hi def link wpsCond             wpsConditional
hi def link wpsIfEndIf          wpsConditional
hi def link wpsElse             wpsConditional

" Tags not otherwise defined (overridden by known tags).
syn match   wpsUnknownTag       "\a\+"                  contained
syn cluster wpsTag              add=wpsUnknownTag
hi def link wpsUnknownTag       wpsTagError


" Known tags are defined individually, so that the user can classify
" and colour them differently if desired.


" %a -- Alignment tags.

syn match   wpsAlignError       "a\a"                   contained
syn cluster wpsTag              add=wpsAlignError
hi def link wpsAlignError       wpsTagError

syn match   wpsAlignTag         "a[lcr]"                contained
syn cluster wpsTag              add=wpsAlignTag
hi def link wpsAlignTag         wpsTag


" %b -- Power tags.

syn match   wpsPowerError       "b\a"                   contained
syn cluster wpsTag              add=wpsPowerError
hi def link wpsPowerError       wpsTagError

syn match   wpsPowerTag         "b[clpstv]"             contained
syn cluster wpsTag              add=wpsPowerTag
hi def link wpsPowerTag         wpsTag


" %c -- Clock tags.

syn match   wpsClockError       "c\a"                   contained
syn cluster wpsTag              add=wpsClockError
hi def link wpsClockError       wpsTagError

syn match   wpsClockTag         "c[abdefklmpuwyHIMPSY]"  contained
syn cluster wpsTag              add=wpsClockTag
hi def link wpsClockTag         wpsTag


" %C -- Album art tags.

syn match   wpsAlbumArtError    "C\a"                   contained
syn cluster wpsTag              add=wpsAlbumArtError
hi def link wpsAlbumArtError    wpsTagError

syn match   wpsDefAlbumArtTag   "Cl|"he=e-1             contained contains=wpsSep nextgroup=wpsDefAlbumArtX
syn match   wpsDefAlbumArtX     "\d\+|"he=e-1           contained contains=wpsSep nextgroup=wpsDefAlbumArtY
syn match   wpsDefAlbumArtY     "\d\+|"he=e-1           contained contains=wpsSep nextgroup=wpsDefAlbumArtW
syn match   wpsDefAlbumArtW     "\([lcrs]\?\d\+\)\?|"he=e-1 contained contains=wpsSep nextgroup=wpsDefAlbumArtH
syn match   wpsDefAlbumArtH     "\([tcbs]\?\d\+\)\?|"he=e-1 contained contains=wpsSep
syn cluster wpsTag              add=wpsDefAlbumArtTag
hi def link wpsDefAlbumArtTag   wpsAlbumArtTags
hi def link wpsDefAlbumArtX     wpsAlbumArtArgs
hi def link wpsDefAlbumArtY     wpsAlbumArtArgs
hi def link wpsDefAlbumArtW     wpsAlbumArtArgs
hi def link wpsDefAlbumArtH     wpsAlbumArtArgs
hi def link wpsAlbumArtArgs     wpsArgs

syn match   wpsRefAlbumArtTag   "C\>"                   contained
syn cluster wpsTag              add=wpsRefAlbumArtTag
hi def link wpsRefAlbumArtTag   wpsAlbumArtTags

hi def link wpsAlbumArtTags     wpsTag


" %d -- Directory tags.

syn match   wpsDirError         "d\a"                   contained
syn match   wpsNextDirError     "D\a"                   contained
syn cluster wpsTag              add=wpsDirError,wpsNextDirError
hi def link wpsDirError         wpsTagError
hi def link wpsNextDirError     wpsTagError

syn match   wpsDirTag           "d[123]"                contained
syn match   wpsNextDirTag       "D[123]"                contained
syn cluster wpsTag              add=wpsDirTag,wpsNextDirTag
hi def link wpsDirTag           wpsDirTags
hi def link wpsNextDirTag       wpsDirTags
hi def link wpsDirTags          wpsTag


" %f -- File tags.

syn match   wpsFileError        "f\a"                   contained
syn match   wpsNextFileError    "F\a"                   contained
syn cluster wpsTag              add=wpsFileError,wpsNextFileError
hi def link wpsFileError        wpsTagError
hi def link wpsNextFileError    wpsTagError

syn match   wpsFileTag          "f[bcfkmnpsv]"          contained
syn match   wpsNextFileTag      "F[bcfkmnpsv]"          contained
syn cluster wpsTag              add=wpsFileTag,wpsNextFileTag
hi def link wpsFileTag          wpsFileTags
hi def link wpsNextFileTag      wpsFileTags
hi def link wpsFileTags         wpsTag


" %i -- ID3 tags.

syn match   wpsID3Error         "i\a"                   contained
syn match   wpsNextID3Error     "I\a"                   contained
syn cluster wpsTag              add=wpsID3Error,wpsNextID3Error
hi def link wpsID3Error         wpsTagError
hi def link wpsNextID3Error     wpsTagError

syn match   wpsID3Tag           "i[acdgntvykAGC]"       contained
syn match   wpsNextID3Tag       "I[acdgntvykAGC]"       contained
syn cluster wpsTag              add=wpsID3Tag,wpsNextID3Tag
hi def link wpsID3Tag           wpsID3Tags
hi def link wpsNextID3Tag       wpsID3Tags
hi def link wpsID3Tags          wpsTag


" %l -- LED tags.

syn match   wpsLEDError         "l\a"                   contained
syn cluster wpsTag              add=wpsLEDError
hi def link wpsLEDError         wpsTagError

syn match   wpsLEDTag           "l[h]"                  contained
syn cluster wpsTag              add=wpsLEDTag
hi def link wpsLEDTag           wpsTag


" %m -- Mode tags.

syn match   wpsModeError        "m\a"                   contained
syn cluster wpsTag              add=wpsModeError
hi def link wpsModeError        wpsTagError

syn match   wpsHoldTag          "m[hr]"                 contained
hi def link wpsHoldTag          wpsTag
syn cluster wpsTag              add=wpsHoldTag

syn match   wpsPlaybackTag      "mp"                    contained
hi def link wpsPlaybackTag      wpsTag
syn cluster wpsTag              add=wpsPlaybackTag

syn match   wpsRepeatTag        "mm"                    contained
hi def link wpsRepeatTag        wpsTag
syn cluster wpsTag              add=wpsRepeatTag

syn match   wpsVolumeTag        "mv"                    contained
hi def link wpsVolumeTag        wpsTag
syn cluster wpsTag              add=wpsVolumeTag


" %p -- Playlist/Song tags.

syn match   wpsSongError        "p\a"                   contained
syn cluster wpsTag              add=wpsSongError
hi def link wpsSongError        wpsTagError

syn match   wpsSongTag          "p[cefmnprstvx]"        contained
syn cluster wpsTag              add=wpsSongTag
hi def link wpsSongTag          wpsTag

syn match   wpsProgress6Tag     "pb"                    contained
syn cluster wpsTag              add=wpsProgress6Tag
hi def link wpsProgress6Tag     wpsTag

syn match   wpsProgressTag      "pb|"                   contained contains=wpsSep nextgroup=wpsProgressFile
syn match   wpsProgressFile     "[^|]\+|"he=e-1         contained contains=wpsSep nextgroup=wpsProgressX
syn match   wpsProgressX        "\(-\|\d\+\)|"he=e-1    contained contains=wpsSep nextgroup=wpsProgressY
syn match   wpsProgressY        "\(-\|\d\+\)|"he=e-1    contained contains=wpsSep nextgroup=wpsProgressW
syn match   wpsProgressW        "\(-\|\d\+\)|"he=e-1    contained contains=wpsSep nextgroup=wpsProgressH
syn match   wpsProgressH        "\(-\|\d\+\)|"he=e-1    contained contains=wpsSep
syn cluster wpsTag              add=wpsProgressTag
hi def link wpsProgressTag      wpsTag
hi def link wpsProgressFile     wpsProgressArgs
hi def link wpsProgressX        wpsProgressArgs
hi def link wpsProgressY        wpsProgressArgs
hi def link wpsProgressW        wpsProgressArgs
hi def link wpsProgressH        wpsProgressArgs
hi def link wpsProgressArgs     wpsArgs


" %r -- Runtime/Replaygain tags.

syn match   wpsRuntimeError     "r\a"                   contained
syn cluster wpsTag              add=wpsRuntimeError
hi def link wpsRuntimeError     wpsTagError

syn match   wpsRuntimeTag       "r[apr]"                contained
syn cluster wpsTag              add=wpsRuntimeTag
hi def link wpsRuntimeTag       wpsTag

syn match   wpsReplaygainTag    "rg"                    contained
syn cluster wpsTag              add=wpsReplaygainTag
hi def link wpsReplaygainTag    wpsTag


" %s -- Scrolling tag.

syn match   wpsScrollError      "s\a"                   contained
syn cluster wpsTag              add=wpsScrollError
hi def link wpsScrollError      wpsTagError

syn match   wpsScrollTag        "s\>"                   contained
syn cluster wpsTag              add=wpsScrollTag
hi def link wpsScrollTag        wpsTag


" %S -- Settings tags.

syn match   wpsSettingError     "S\a"                   contained
syn cluster wpsTag              add=wpsSettingError
hi def link wpsSettingError     wpsTagError

syn match   wpsSettingTag       "St|"he=e-1             contained contains=wpsSep nextgroup=wpsSettingName
syn match   wpsSettingName      "[^|]\+|"he=e-1         contained contains=wpsSep
syn cluster wpsTag              add=wpsSettingTag
hi def link wpsSettingTag       wpsTag
hi def link wpsSettingName      wpsSettingArgs
hi def link wpsSettingArgs      wpsArgs

syn match   wpsPitchTag         "Sp\>"                  contained
syn cluster wpsTag              add=wpsPitchTag
hi def link wpsPitchTag         wpsTag

syn match   wpsTranslatedTag    "Sx|"he=e-1             contained contains=wpsSep nextgroup=wpsTranslatedText
syn match   wpsTranslatedText   "[^|]\+|"he=e-1         contained contains=wpsSep
syn cluster wpsTag              add=wpsTranslatedTag
hi def link wpsTranslatedTag    wpsTag
hi def link wpsTranslatedText   wpsTranslatedArgs
hi def link wpsTranslatedArgs   wpsArgs

syn match   wpsLangIsRtlTag     "Sr"                    contained
hi def link wpsLangIsRtlTag     wpsArgs
syn cluster wpsTag              add=wpsLangIsRTLTag

" %t -- Alternation tags.

syn match   wpsAlternateError   "t"                     contained
syn cluster wpsTag              add=wpsAlternateError
hi def link wpsAlternateError   wpsTagError

syn match   wpsAlternateTag     "t\d"me=e-1             contained contains=wpsSep nextgroup=wpsAlternateTime
syn match   wpsAlternateTime    "\d\+\(\.\d+\)\?"       contained
syn cluster wpsTag              add=wpsAlternateTag
hi def link wpsAlternateTag     wpsTag
hi def link wpsAlternateTime    wpsAlternateArgs
hi def link wpsAlternateArgs    wpsArgs


" %V -- Viewport tags.

syn match   wpsViewportError    "V\a"                   contained
syn cluster wpsTag              add=wpsViewportError
hi def link wpsViewportError    wpsTagError

syn match   wpsViewportTag      "V|"he=e-1              contained contains=wpsSep nextgroup=wpsViewportX
syn cluster wpsTag              add=wpsViewportTag
hi def link wpsViewportTag      wpsViewportTags

syn match   wpsDefViewportTag   "Vl|"he=e-1             contained contains=wpsSep nextgroup=wpsDefViewportId
syn match   wpsDefViewportId    "\a|"he=e-1             contained contains=wpsSep nextgroup=wpsViewportX
syn match   wpsViewportX        "\d\+|"he=e-1           contained contains=wpsSep nextgroup=wpsViewportY
syn match   wpsViewportY        "\d\+|"he=e-1           contained contains=wpsSep nextgroup=wpsViewportW
syn match   wpsViewportW        "\(-\|\d\+\)|"he=e-1    contained contains=wpsSep nextgroup=wpsViewportH
syn match   wpsViewportH        "\(-\|\d\+\)|"he=e-1    contained contains=wpsSep nextgroup=wpsViewportFont
syn match   wpsViewportFont     "[-01]|"he=e-1          contained contains=wpsSep nextgroup=wpsViewportShade,wpsViewportFG
syn match   wpsViewportShade    "[-0-3]|"he=e-1         contained contains=wpsSep
syn match   wpsViewportFG       "\(-\|\x\{6}\)|"he=e-1  contained contains=wpsSep nextgroup=wpsViewportBG
syn match   wpsViewportBG       "\(-\|\x\{6}\)|"he=e-1  contained contains=wpsSep
syn cluster wpsTag              add=wpsDefViewportTag
hi def link wpsDefViewportId    wpsDefId
hi def link wpsDefViewportTag   wpsViewportTags
hi def link wpsViewportX        wpsViewportArgs
hi def link wpsViewportY        wpsViewportArgs
hi def link wpsViewportW        wpsViewportArgs
hi def link wpsViewportH        wpsViewportArgs
hi def link wpsViewportFont     wpsViewportArgs
hi def link wpsViewportShade    wpsViewportArgs
hi def link wpsViewportFG       wpsViewportArgs
hi def link wpsViewportBG       wpsViewportArgs
hi def link wpsViewportArgs     wpsArgs

syn match   wpsRefViewportTag   "Vd"                    nextgroup=wpsRefViewportId
syn match   wpsRefViewportId    "\a"                    contained
syn cluster wpsTag              add=wpsRefViewportTag
hi def link wpsRefViewportTag   wpsViewportTags
hi def link wpsRefViewportId    wpsRefId

hi def link wpsViewportTags     wpsTag


" %w -- Status bar tags.

syn match   wpsStatusBarError   "w\a"                   contained
syn cluster wpsTag              add=wpsStatusBarError
hi def link wpsStatusBarError   wpsTagError

syn match   wpsStatusBarTag     "w[ed]"                 contained
syn cluster wpsTag              add=wpsStatusBarTag
hi def link wpsStatusBarTag     wpsTag


" %x -- Image/Crossfade tags.

syn match   wpsImageError       "x\a"                   contained
syn cluster wpsTag              add=wpsImageError
hi def link wpsImageError       wpsTagError

syn match   wpsLoadImageTag     "xl|"he=e-1             contained contains=wpsSep nextgroup=wpsLoadImageId
syn match   wpsLoadImageId      "\a|"he=e-1             contained contains=wpsSep nextgroup=wpsLoadImageFile
syn match   wpsLoadImageFile    "[^|]\+|"he=e-1         contained contains=wpsSep nextgroup=wpsLoadImageX
syn match   wpsLoadImageX       "\d\+|"he=e-1           contained contains=wpsSep nextgroup=wpsLoadImageY
syn match   wpsLoadImageY       "\d\+|"he=e-1           contained contains=wpsSep
syn cluster wpsTag              add=wpsLoadImageTag
hi def link wpsLoadImageTag     wpsImageTags
hi def link wpsLoadImageFile    wpsImageArgs
hi def link wpsLoadImageX       wpsImageArgs
hi def link wpsLoadImageY       wpsImageArgs

syn match   wpsDefImageTag      "xl|"he=e-1             contained contains=wpsSep nextgroup=wpsDefImageId
syn match   wpsDefImageId       "\a|"he=e-1             contained contains=wpsSep nextgroup=wpsDefImageFile
syn match   wpsDefImageFile     "[^|]\+|"he=e-1         contained contains=wpsSep nextgroup=wpsDefImageX
syn match   wpsDefImageX        "\d\+|"he=e-1           contained contains=wpsSep nextgroup=wpsDefImageY
syn match   wpsDefImageY        "\d\+|"he=e-1           contained contains=wpsSep nextgroup=wpsDefImageCount
syn match   wpsDefImageCount    "\d\+|"he=e-1           contained contains=wpsSep
syn cluster wpsTag              add=wpsDefImageTag
hi def link wpsDefImageTag      wpsImageTags
hi def link wpsDefImageId       wpsDefId
hi def link wpsDefImageFile     wpsImageArgs
hi def link wpsDefImageX        wpsImageArgs
hi def link wpsDefImageY        wpsImageArgs
hi def link wpsDefImageCount    wpsImageArgs

syn match   wpsRefImageTag      "xd"                    nextgroup=wpsRefImageId
syn match   wpsRefImageId       "\a"                    contained nextgroup=wpsRefImageBitmap
syn match   wpsRefImageBitmap   "\a"                    contained
syn cluster wpsTag              add=wpsRefImageTag
hi def link wpsRefImageTag      wpsImageTags
hi def link wpsRefImageId       wpsRefId
hi def link wpsRefImageBitmap   wpsRefId

hi def link wpsImageTags        wpsTag
hi def link wpsImageArgs        wpsArgs


syn match   wpsCrossfadeTag     "xf"                    contained
syn cluster wpsTag              add=wpsCrossfadeTag
hi def link wpsCrossfadeTag     wpsTag


" %X -- Backdrop tag.

syn match   wpsBackdropError    "X\a"                   contained
syn cluster wpsTag              add=wpsBackdropError
hi def link wpsBackdropError    wpsTagError

syn match   wpsBackdropTag      "X|"                    contained contains=wpsSep nextgroup=wpsBackdropFile
syn match   wpsBackdropFile     "[^|]\+|"he=e-1         contained contains=wpsSep
syn cluster wpsTag              add=wpsBackdropTag
hi def link wpsBackdropTag      wpsTag
hi def link wpsBackdropFile     wpsBackdropArgs
hi def link wpsBackdropArgs     wpsArgs



hi def link wpsDefId            wpsIdentifier
hi def link wpsRefId            wpsIdentifier

hi def link wpsPct              wpsOperator
hi def link wpsSep              wpsOperator
hi def link wpsSubline          wpsOperator

hi def link wpsPipeError        wpsError
hi def link wpsEndIfError       wpsError
hi def link wpsTagError         wpsError
hi def link wpsSpaceError       wpsError

hi def link wpsError            Error
hi def link wpsComment          Comment
hi def link wpsTodo             Todo
hi def link wpsSpecial          SpecialChar
hi def link wpsTag              Statement
hi def link wpsArgs             Constant
hi def link wpsOperator         Operator
hi def link wpsConditional      Conditional
hi def link wpsIdentifier       Identifier

let b:current_syntax = "wps"

" vim:ts=4 et sts=4 sw=4:
