
Description
-----------
The core Search module provides a simple list of top search phrases from the
watchdog log using the function dblog_top. However, most sites clear watchdog 
frequently. Search Log stores search terms indefinitely and provides more 
robust reporting.

* Search Log supports all modules which implement hook_search(). Modules can 
  be configured to be excluded from logging.

* Search Log supports the search page, search block and theme search forms.
  By default, logging is performed at form submit before the search results are
  generated, but developers can implement a search_log hook to add additional 
  entries to the log or record failed searches.

* (new) Search Log has an experimental feature to attempt to capture failed 
  searches. While most content reporting revolves around identifying what is 
  popular, identifying failed searches allows you to know what users are 
  looking for and not finding (and then potentially going elsewhere).

* Search Log table can be truncated at a user-specified interval or kept 
  indefinitely.

* Search Log reporting can be filtered by date, module and status.

* Search Log provides a configurable block of Top Searches.


Prerequisites
-------------
Search module.


Usage
-------------
After enabling the module, Search Log is configured at:
admin/settings/search

The improved Search log can be viewed at: 
admin/reports/search

The Top Searches block can be enabled and configured at:
admin/build/block


Bugs/Features/Patches:
----------------------
If you want to report bugs, feature requests, or submit a patch, please do so 
at the project page on the Drupal web site.
http://drupal.org/project/search_log


Author
------
John Money
ossemble LLC.
http://ossemble.com

Module development sponsored by Consumer Search, a service of About.com, a part 
of The New York Times Company.
http://www.consumersearch.com
