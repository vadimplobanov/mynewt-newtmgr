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

package config

import (
	"mynewt.apache.org/newtmgr/newtmgr/nmutil"
	"mynewt.apache.org/newtmgr/nmxact/nmdds"
	"mynewt.apache.org/newt/util"
)

func ParseDdsConnString(cs string) (*nmdds.XportCfg, error) {
	if cs == "" {
		return nil, util.FmtNewtError("Invalid dds connstring; cannot be empty")
	}

	dc := nmdds.NewXportCfg()
	dc.CommTimeout = nmutil.TxOptions().Timeout
	dc.TargetMatch = cs

	return dc, nil
}
