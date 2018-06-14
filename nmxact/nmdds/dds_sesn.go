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

package nmdds

import (
	"fmt"
	"sync"

	"mynewt.apache.org/newtmgr/nmxact/mgmt"
	"mynewt.apache.org/newtmgr/nmxact/sesn"
)

type DdsSesn struct {
	cfg    sesn.SesnCfg
	dx     *DdsXport
	txvr   *mgmt.Transceiver

	isopen bool
	mopen  sync.Mutex
}

func NewDdsSesn(dx *DdsXport, cfg sesn.SesnCfg) (*DdsSesn, error) {
	if cfg.MgmtProto != sesn.MGMT_PROTO_NMP {
		return nil, fmt.Errorf("Invalid operation for %s", cfg.MgmtProto)
	}

	txvr, err := mgmt.NewTransceiver(cfg.TxFilterCb, cfg.RxFilterCb, false, cfg.MgmtProto, 3)
	if err != nil {
		return nil, err
	}

	s := &DdsSesn{}

	s.cfg = cfg
	s.dx = dx
	s.txvr = txvr
	s.isopen = false

	return s, nil
}
