/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#include "ContentTypes.h"

using namespace std;
using namespace cb;
using namespace cb::HTTP;


#define OPENDOC "application/vnd.oasis.opendocument"
#define OPENXML "application/vnd.openxmlformats-officedocument"


ContentTypes::ContentTypes(Inaccessible) {
  insert(value_type("atom",    "application/atom+xml"));
  insert(value_type("es",      "application/ecmascript"));
  insert(value_type("epub",    "application/epub+zip"));
  insert(value_type("jar",     "application/java-archive"));
  insert(value_type("class",   "application/java-vm"));
  insert(value_type("js",      "application/javascript"));
  insert(value_type("json",    "application/json"));
  insert(value_type("doc",     "application/msword"));
  insert(value_type("bin",     "application/octet-stream"));
  insert(value_type("pdf",     "application/pdf"));
  insert(value_type("pgp",     "application/pgp-encrypted"));
  insert(value_type("p10",     "application/pkcs10"));
  insert(value_type("p7m",     "application/pkcs7-mime"));
  insert(value_type("p7s",     "application/pkcs7-signature"));
  insert(value_type("p8",      "application/pkcs8"));
  insert(value_type("pls",     "application/pls+xml"));
  insert(value_type("ai",      "application/postscript"));
  insert(value_type("rdf",     "application/rdf+xml"));
  insert(value_type("rsd",     "application/rsd+xml"));
  insert(value_type("rss",     "application/rss+xml"));
  insert(value_type("rtf",     "application/rtf"));
  insert(value_type("xls",     "application/vnd.ms-excel"));
  insert(value_type("ppt",     "application/vnd.ms-powerpoint"));
  insert(value_type("odc",     OPENDOC ".chart"));
  insert(value_type("otc",     OPENDOC ".chart-template"));
  insert(value_type("odb",     OPENDOC ".database"));
  insert(value_type("odf",     OPENDOC ".formula"));
  insert(value_type("odft",    OPENDOC ".formula-template"));
  insert(value_type("odg",     OPENDOC ".graphics"));
  insert(value_type("otg",     OPENDOC ".graphics-template"));
  insert(value_type("odi",     OPENDOC ".image"));
  insert(value_type("oti",     OPENDOC ".image-template"));
  insert(value_type("odp",     OPENDOC ".presentation"));
  insert(value_type("otp",     OPENDOC ".presentation-template"));
  insert(value_type("ods",     OPENDOC ".spreadsheet"));
  insert(value_type("ots",     OPENDOC ".spreadsheet-template"));
  insert(value_type("odt",     OPENDOC ".text"));
  insert(value_type("odm",     OPENDOC ".text-master"));
  insert(value_type("ott",     OPENDOC ".text-template"));
  insert(value_type("oth",     OPENDOC ".text-web"));
  insert(value_type("pptx",    OPENXML ".presentationml.presentation"));
  insert(value_type("sldx",    OPENXML ".presentationml.slide"));
  insert(value_type("ppsx",    OPENXML ".presentationml.slideshow"));
  insert(value_type("potx",    OPENXML ".presentationml.template"));
  insert(value_type("xlsx",    OPENXML ".spreadsheetml.sheet"));
  insert(value_type("xltx",    OPENXML ".spreadsheetml.template"));
  insert(value_type("docx",    OPENXML ".wordprocessingml.document"));
  insert(value_type("dotx",    OPENXML ".wordprocessingml.template"));
  insert(value_type("7z",      "application/x-7z-compressed"));
  insert(value_type("dmg",     "application/x-apple-diskimage"));
  insert(value_type("torrent", "application/x-bittorrent"));
  insert(value_type("bz",      "application/x-bzip"));
  insert(value_type("bz2",     "application/x-bzip2"));
  insert(value_type("cpio",    "application/x-cpio"));
  insert(value_type("deb",     "application/x-debian-package"));
  insert(value_type("wad",     "application/x-doom"));
  insert(value_type("dvi",     "application/x-dvi"));
  insert(value_type("gsf",     "application/x-font-ghostscript"));
  insert(value_type("psf",     "application/x-font-linux-psf"));
  insert(value_type("otf",     "application/x-font-otf"));
  insert(value_type("pcf",     "application/x-font-pcf"));
  insert(value_type("snf",     "application/x-font-snf"));
  insert(value_type("ttf",     "application/x-font-ttf"));
  insert(value_type("pfa",     "application/x-font-type1"));
  insert(value_type("woff",    "application/x-font-woff"));
  insert(value_type("gz",      "application/x-gzip"));
  insert(value_type("latex",   "application/x-latex"));
  insert(value_type("p12",     "application/x-pkcs12"));
  insert(value_type("p7b",     "application/x-pkcs7-certificates"));
  insert(value_type("p7r",     "application/x-pkcs7-certreqresp"));
  insert(value_type("rar",     "application/x-rar-compressed"));
  insert(value_type("sh",      "application/x-sh"));
  insert(value_type("shar",    "application/x-shar"));
  insert(value_type("swf",     "application/x-shockwave-flash"));
  insert(value_type("sit",     "application/x-stuffit"));
  insert(value_type("sitx",    "application/x-stuffitx"));
  insert(value_type("tar",     "application/x-tar"));
  insert(value_type("tcl",     "application/x-tcl"));
  insert(value_type("tex",     "application/x-tex"));
  insert(value_type("tfm",     "application/x-tex-tfm"));
  insert(value_type("texinfo", "application/x-texinfo"));
  insert(value_type("ustar",   "application/x-ustar"));
  insert(value_type("crt",     "application/x-x509-ca-cert"));
  insert(value_type("der",     "application/x-x509-ca-cert"));
  insert(value_type("fig",     "application/x-xfig"));
  insert(value_type("xhtml",   "application/xhtml+xml"));
  insert(value_type("xml",     "application/xml"));
  insert(value_type("dtd",     "application/xml-dtd"));
  insert(value_type("xslt",    "application/xslt+xml"));
  insert(value_type("zip",     "application/zip"));
  insert(value_type("adp",     "audio/adpcm"));
  insert(value_type("au",      "audio/basic"));
  insert(value_type("mid",     "audio/midi"));
  insert(value_type("mp4a",    "audio/mp4"));
  insert(value_type("mpga",    "audio/mpeg"));
  insert(value_type("oga",     "audio/ogg"));
  insert(value_type("aac",     "audio/x-aac"));
  insert(value_type("aif",     "audio/x-aiff"));
  insert(value_type("m3u",     "audio/x-mpegurl"));
  insert(value_type("wma",     "audio/x-ms-wma"));
  insert(value_type("wav",     "audio/x-wav"));
  insert(value_type("bmp",     "image/bmp"));
  insert(value_type("gif",     "image/gif"));
  insert(value_type("jpeg",    "image/jpeg"));
  insert(value_type("jpg",     "image/jpeg"));
  insert(value_type("png",     "image/png"));
  insert(value_type("svg",     "image/svg+xml"));
  insert(value_type("tiff",    "image/tiff"));
  insert(value_type("psd",     "image/vnd.adobe.photoshop"));
  insert(value_type("dwg",     "image/vnd.dwg"));
  insert(value_type("dxf",     "image/vnd.dxf"));
  insert(value_type("ico",     "image/x-icon"));
  insert(value_type("pnm",     "image/x-portable-anymap"));
  insert(value_type("pbm",     "image/x-portable-bitmap"));
  insert(value_type("pgm",     "image/x-portable-graymap"));
  insert(value_type("ppm",     "image/x-portable-pixmap"));
  insert(value_type("xbm",     "image/x-xbitmap"));
  insert(value_type("xpm",     "image/x-xpixmap"));
  insert(value_type("igs",     "model/iges"));
  insert(value_type("msh",     "model/mesh"));
  insert(value_type("wrl",     "model/vrml"));
  insert(value_type("ics",     "text/calendar"));
  insert(value_type("css",     "text/css"));
  insert(value_type("csv",     "text/csv"));
  insert(value_type("htm",     "text/html"));
  insert(value_type("html",    "text/html"));
  insert(value_type("c",       "text/plain"));
  insert(value_type("c++",     "text/plain"));
  insert(value_type("cpp",     "text/plain"));
  insert(value_type("h",       "text/plain"));
  insert(value_type("hpp",     "text/plain"));
  insert(value_type("py",      "text/plain"));
  insert(value_type("txt",     "text/plain"));
  insert(value_type("rtx",     "text/richtext"));
  insert(value_type("sgml",    "text/sgml"));
  insert(value_type("tsv",     "text/tab-separated-values"));
  insert(value_type("t",       "text/troff"));
  insert(value_type("uri",     "text/uri-list"));
  insert(value_type("s",       "text/x-asm"));
  insert(value_type("f",       "text/x-fortran"));
  insert(value_type("java",    "text/x-java-source"));
  insert(value_type("p",       "text/x-pascal"));
  insert(value_type("vcs",     "text/x-vcalendar"));
  insert(value_type("vcf",     "text/x-vcard"));
  insert(value_type("yaml",    "text/yaml"));
  insert(value_type("mp4",     "video/mp4"));
  insert(value_type("mpeg",    "video/mpeg"));
  insert(value_type("ogv",     "video/ogg"));
  insert(value_type("qt",      "video/quicktime"));
  insert(value_type("webm",    "video/webm"));
  insert(value_type("flv",     "video/x-flv"));
  insert(value_type("m4v",     "video/x-m4v"));
  insert(value_type("asf",     "video/x-ms-asf"));
  insert(value_type("wm",      "video/x-ms-wm"));
  insert(value_type("wmv",     "video/x-ms-wmv"));
  insert(value_type("wmx",     "video/x-ms-wmx"));
  insert(value_type("wvx",     "video/x-ms-wvx"));
  insert(value_type("avi",     "video/x-msvideo"));
}
