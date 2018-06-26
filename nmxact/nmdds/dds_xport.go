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

// #cgo LDFLAGS: -L ../../ddslib -l ddsmgr
// #include <stdlib.h>
// #include "../../ddslib/src/ddsmgr.h"
import "C"

import (
	"fmt"
	"math/rand"
	"regexp"
	"sync"
	"time"
	"unsafe"

	"mynewt.apache.org/newtmgr/nmxact/sesn"
)

type XportCfg struct {
	CommTimeout time.Duration
	TargetMatch string
	Mtu         int
}

func NewXportCfg() *XportCfg {
	return &XportCfg{
		CommTimeout: 10 * time.Second,
		TargetMatch: "",
		Mtu:         512,
	}
}

type DdsXport struct {
	cfg     *XportCfg
	devname string
	mutex   sync.Mutex
	closing bool
	rspsesn *DdsSesn
}

func NewDdsXport(cfg *XportCfg) *DdsXport {
	x := &DdsXport{}

	x.cfg = cfg

	return x
}

func (dx *DdsXport) setRspSesn(s *DdsSesn) error {
	dx.mutex.Lock()
	defer dx.mutex.Unlock()

	if dx.closing {
		return fmt.Errorf("Transport dds closed")
	}

	if s != nil && dx.rspsesn != nil {
		return fmt.Errorf("Transport dds busy")
	}

	dx.rspsesn = s
	return nil
}

func (dx *DdsXport) Start() error {
	rand.Seed(time.Now().Unix())

	dx.mutex.Lock()
	defer dx.mutex.Unlock()

	abstimeout := C.struct_abs_timeout{}
	packetping := C.struct_packet_ping{}
	packetpong := C.struct_packet_pong{}

	timeout := time.Now().Add(dx.cfg.CommTimeout)
	abstimeout.seconds = C.ulong(timeout.Unix())
	abstimeout.nseconds = C.long(timeout.Nanosecond())

	rc := C.ddsmgr_initialize(&abstimeout)
	if rc != 0 {
		return fmt.Errorf("Failed to initialize ddsmgr library")
	}

	targetmatch := regexp.MustCompile(dx.cfg.TargetMatch)
	devicefound := false

	requestid := rand.Int31()
	packetping.request_id = C.int(requestid)

	C.ddsmgr_pong_listen()
	C.ddsmgr_ping_send(&packetping)
	for {
		rc = C.ddsmgr_pong_recv(&abstimeout, &packetpong)
		if rc != 0 {
			break
		}

		in_requestid := int32(packetpong.request_id)
		in_devicename := C.GoString(&packetpong.device_name[0])

		if in_requestid != requestid {
			continue
		}

		if targetmatch.MatchString(in_devicename) {
			dx.devname = in_devicename
			devicefound = true
			break
		}
	}
	C.ddsmgr_pong_reject()

	if !devicefound {
		return fmt.Errorf("Could not find matching dds device")
	}

	fmt.Println("Matched dds device", dx.devname)

	return nil
}

func (dx *DdsXport) Stop() error {
	dx.mutex.Lock()
	defer dx.mutex.Unlock()

	dx.closing = true
	return nil
}

func (dx *DdsXport) BuildSesn(cfg sesn.SesnCfg) (sesn.Sesn, error) {
	return NewDdsSesn(dx, cfg)
}

func (dx *DdsXport) Tx(bytes []byte) error {
	abstimeout := C.struct_abs_timeout{}
	packetmcmd := C.struct_packet_mcmd{}
	packetmrsp := C.struct_packet_mrsp{}

	timeout := time.Now().Add(dx.cfg.CommTimeout)
	abstimeout.seconds = C.ulong(timeout.Unix())
	abstimeout.nseconds = C.long(timeout.Nanosecond())

	requestid := rand.Int31()
	devicename := C.CString(dx.devname)
	defer C.free(unsafe.Pointer(devicename))
	cmddata := C.CBytes(bytes)
	defer C.free(unsafe.Pointer(cmddata))
	packetmcmd.request_id = C.int(requestid)
	packetmcmd.device_name = devicename
	packetmcmd.cmd_data = (*C.char)(unsafe.Pointer(cmddata))
	packetmcmd.cmd_size = C.int(len(bytes))

	var rc C.int
	var rsp []byte

	C.ddsmgr_mrsp_listen()
	C.ddsmgr_mcmd_send(&packetmcmd)
	for {
		rc = C.ddsmgr_mrsp_recv(&abstimeout, &packetmrsp)
		if rc != 0 {
			break
		}

		in_requestid := int32(packetmrsp.request_id)
		in_rspdata := C.GoBytes(unsafe.Pointer(&packetmrsp.rsp_data[0]),
		                        packetmrsp.rsp_size)

		if in_requestid == requestid {
			rsp = in_rspdata
			break
		}
	}
	C.ddsmgr_mrsp_reject()

	if rc != 0 {
		return fmt.Errorf("Did not receive a dds command response")
	}

	dx.mutex.Lock()
	rspsesn := dx.rspsesn
	dx.mutex.Unlock()

	if rspsesn == nil {
		return fmt.Errorf("Received dds response with no session")
	}

	return rspsesn.rx(rsp)
}
