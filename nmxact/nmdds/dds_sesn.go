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

	"github.com/runtimeco/go-coap"

	"mynewt.apache.org/newtmgr/nmxact/mgmt"
	"mynewt.apache.org/newtmgr/nmxact/nmcoap"
	"mynewt.apache.org/newtmgr/nmxact/nmp"
	"mynewt.apache.org/newtmgr/nmxact/nmxutil"
	"mynewt.apache.org/newtmgr/nmxact/omp"
	"mynewt.apache.org/newtmgr/nmxact/sesn"
)

type DdsSesn struct {
	cfg    sesn.SesnCfg
	dx     *DdsXport
	txvr   *mgmt.Transceiver
	mopen  sync.Mutex
	isopen bool
}

func NewDdsSesn(dx *DdsXport, cfg sesn.SesnCfg) (*DdsSesn, error) {
	if cfg.MgmtProto != sesn.MGMT_PROTO_NMP {
		return nil, fmt.Errorf("Invalid operation for %s", cfg.MgmtProto)
	}

	txvr, err := mgmt.NewTransceiver(cfg.TxFilterCb, cfg.RxFilterCb,
					 false, cfg.MgmtProto, 3)
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

func (s *DdsSesn) Open() error {
	s.mopen.Lock()

	if s.isopen {
		s.mopen.Unlock()
		return nmxutil.NewSesnAlreadyOpenError(
			"Attempt to open an already-open dds session")
	}

	txvr, err := mgmt.NewTransceiver(s.cfg.TxFilterCb, s.cfg.RxFilterCb,
					 false, s.cfg.MgmtProto, 3)
	if err != nil {
		s.mopen.Unlock()
		return err
	}
	s.txvr = txvr

	s.isopen = true
	s.mopen.Unlock()

	return nil
}

func (s *DdsSesn) Close() error {
	s.mopen.Lock()

	if !s.isopen {
		s.mopen.Unlock()
		return nmxutil.NewSesnClosedError(
			"Attempt to close an unopened dds session")
	}

	s.txvr.ErrorAll(fmt.Errorf("closed"))
	s.txvr.Stop()
	s.txvr = nil

	s.isopen = false
	s.mopen.Unlock()

	return nil
}

func (s *DdsSesn) IsOpen() bool {
	s.mopen.Lock()
	defer s.mopen.Unlock()

	return s.isopen
}

func (s *DdsSesn) MtuIn() int {
	return 1024 - omp.OMP_MSG_OVERHEAD
}

func (s *DdsSesn) MtuOut() int {
	// Mynewt commands have a default chunk buffer size of 512.  Account for
	// base64 encoding.
	return s.dx.cfg.Mtu*3/4 - omp.OMP_MSG_OVERHEAD
}

func (s *DdsSesn) MgmtProto() sesn.MgmtProto {
	return s.cfg.MgmtProto
}

func (s *DdsSesn) CoapIsTcp() bool {
	return false
}

func (s *DdsSesn) AbortRx(nmpSeq uint8) error {
	s.txvr.ErrorAll(fmt.Errorf("Rx aborted"))
	return nil
}

func (s *DdsSesn) RxAccept() (sesn.Sesn, *sesn.SesnCfg, error) {
	return nil, nil, fmt.Errorf("Invalid operation for %s", s.cfg.MgmtProto)
}

func (s *DdsSesn) RxCoap(opt sesn.TxOptions) (coap.Message, error) {
	return nil, fmt.Errorf("Invalid operation for %s", s.cfg.MgmtProto)
}

func (s *DdsSesn) TxNmpOnce(m *nmp.NmpMsg, opt sesn.TxOptions) (nmp.NmpRsp, error) {
	s.mopen.Lock()
	if !s.isopen {
		s.mopen.Unlock()
		return nil, nmxutil.NewSesnClosedError(
			"Attempt to transmit over closed dds session")
	}
	s.mopen.Unlock()

	txFn := func(bytes []byte) error {
		return s.dx.Tx(bytes)
	}

	err := s.dx.setRspSesn(s)
	if err != nil {
		return nil, err
	}
	defer s.dx.setRspSesn(nil)

	return s.txvr.TxNmp(txFn, m, s.MtuOut(), opt.Timeout)
}

func (s *DdsSesn) TxCoapOnce(m coap.Message, resType sesn.ResourceType, opt sesn.TxOptions) (coap.COAPCode, []byte, error) {
	return 0, nil, fmt.Errorf("Invalid operation for %s", s.cfg.MgmtProto)
}

func (s *DdsSesn) TxCoapObserve(m coap.Message, resType sesn.ResourceType, opt sesn.TxOptions, notify sesn.GetNotifyCb, stopsignal chan int) (coap.COAPCode, []byte, []byte, error) {
	return 0, nil, nil, fmt.Errorf("Invalid operation for %s", s.cfg.MgmtProto)
}

func (s *DdsSesn) Filters() (nmcoap.MsgFilter, nmcoap.MsgFilter) {
	return s.cfg.TxFilterCb, s.cfg.RxFilterCb
}
