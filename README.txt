
Recommended/Required modules:
https://drupal.org/node/27882
https://drupal.org/project/disable_all_forms
https://drupal.org/project/expire
https://drupal.org/project/httprl
https://drupal.org/project/xmlsitemap

To rsync the files, be sure to use the -L (transform symlink into referent file/dir)
option as it will follow symlinks.

Also be sure to turn on aggregation of css and js so that all the assets are
moved to the files directory.

Run the static site under apache so the .htaccess works for the redirects.

It is recommended to put most universal pieces (blocks) into Edge side includes.
This will allow them to be included on page delivery instead of needing to 
rerender the entire site when one of them changes. Common includes are the
main menu, header and footer menus. Others are page blocks with non-static or
shared content.
Edge Side Includes:
Apache:
See http://httpd.apache.org/docs/current/howto/ssi.html
ensure the .htaccess file is set up properly and contains the two redirects.
Use SSI
Ensure mod_include is enabled in httpd.conf
Set "XBitHack full" in httpd.conf or other config file. (This allows SSI in files
with execute bit set.)
Change all .html files to have the execute bit set chmod -R a+x *.html

https://drupal.org/project/esi
