/**
 *     Copyright 2016-2017 Couchbase, Inc.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef PHP_COUCHBASE_H_
#define PHP_COUCHBASE_H_

#define PHP_COUCHBASE_VERSION "2.5.1"
#define PHP_COUCHBASE_EXTNAME "couchbase"

extern zend_module_entry couchbase_module_entry;
#define phpext_couchbase_ptr &couchbase_module_entry

#endif
