$Id$


Description
-----------
The core Search module provides a simple list of top search phrases from the
watchdog log using the function watchdog_top. However, most sites clear 
watchdog frequently. Search Log stores search terms indefinitely and provides 
more robust reporting.

* Search Log supports all modules which implement hook_search(). Modules can 
  be configured to be excluded from logging.

* Search Log supports the search page, search block and theme search forms.
  By default, logging is performed at form submit before the search results are
  generated, but developers can implement a search_log hook to add additional 
  entries to the log or record failed searches.

* Search Log table can be truncated at a user-specified interval or kept 
  indefinitely.

* Search Log reporting can be filtered by date, module and (if implemented by 
  a developer) status.

* Search Log provides a configurable block of Top Searches.


Prerequisites
-------------
Search.module is enabled.


Installation
------------
1. copy the search_log directory and all its contents to your 
   modules directory.
2. enable the module: admin/build/modules. The module will install one table
   into your database.
3. configure the module: admin/settings/search/search_log


Usage
-------------
After enabling the module, the Search Log report can be viewed at: 
admin/logs/search

Additionally, the Top Searches block can be enabled and configured at:
admin/build/block


Known Issues
-------------
None


Bugs/Features/Patches:
----------------------
If you want to report bugs, feature requests, or submit a patch, please do so 
at the project page on the Drupal web site.
http://drupal.org/project/search_log


Changlog:
----------
1.0  initial release


Todo List:
----------
None


Author
------
John Money
ossemble LLC.
http://ossemble.com

Module development sponsored by Consumer Search, a service of About.com, a part 
of The New York Times Company.
http://www.consumersearch.com
