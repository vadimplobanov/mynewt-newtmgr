/**
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
 */

package pkg

import (
	"path/filepath"
	"sort"

	"mynewt.apache.org/newt/newt/interfaces"
)

type lpkgSorter struct {
	pkgs []*LocalPackage
}

func (s lpkgSorter) Len() int {
	return len(s.pkgs)
}
func (s lpkgSorter) Swap(i, j int) {
	s.pkgs[i], s.pkgs[j] = s.pkgs[j], s.pkgs[i]
}
func (s lpkgSorter) Less(i, j int) bool {
	return s.pkgs[i].FullName() < s.pkgs[j].FullName()
}

func SortLclPkgs(pkgs []*LocalPackage) []*LocalPackage {
	sorter := lpkgSorter{
		pkgs: make([]*LocalPackage, 0, len(pkgs)),
	}

	for _, p := range pkgs {
		sorter.pkgs = append(sorter.pkgs, p)
	}

	sort.Sort(sorter)
	return sorter.pkgs
}

func ShortName(p interfaces.PackageInterface) string {
	return filepath.Base(p.Name())
}
