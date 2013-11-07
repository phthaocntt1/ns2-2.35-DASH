set val(chan)           Channel/WirelessChannel    ;#Channel Type
set val(prop)           Propagation/TwoRayGround   ;# radio-propagation model
set val(netif)          Phy/WirelessPhy            ;# network interface type
set val(mac)            Mac/802_11                 ;# MAC type
set val(ifq)            Queue/DropTail/PriQueue	   ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)         50                        ;# max packet in ifq
set val(nn)             5                         ;# number of mobilenodes
set val(rp)             DumbAgent                  ;# routing protocol
set val(x)		1000
set val(y)		1000



#Mac/802_11 set dataRate_ 11Mb
Mac/802_11 set CWMin_               15
Mac/802_11 set CWMax_               1023
Mac/802_11 set SlotTime_            0.000009
Mac/802_11 set SIFS_                0.000016
Mac/802_11 set ShortRetryLimit_     7
Mac/802_11 set LongRetryLimit_      4
Mac/802_11 set PreambleLength_      60
Mac/802_11 set PLCPHeaderLength_    60
Mac/802_11 set PLCPDataRate_        6.0e6
Mac/802_11 set RTSThreshold_        3000
Mac/802_11 set basicRate_           1Mb             ;#rate for control frames
Mac/802_11 set dataRate_            11Mb            ;#rate for data frames

Antenna/OmniAntenna set Gt_ 1                       ;#Transmit antenna gain 
Antenna/OmniAntenna set Gr_ 1                       ;#Receive antenna gain 


Phy/WirelessPhy set L_ 1.0                          ;#System Loss Factor 
Phy/WirelessPhy set freq_ 2.472e9                   ;#channel-13. 2.472GHz

#Phy/WirelessPhy set CSThresh_       6.30957e-12
#Phy/WirelessPhy set Pt_             0.031622777    ;#transmit power
#Phy/WirelessPhy set RXThresh_       3.652e-10
#Phy/WirelessPhy set CPThresh_       10.0

Phy/WirelessPhy set bandwidth_      11Mb
#Phy/WirelessPhy set Pt_ 7.214e-2                     ;#Transmit Power
#Phy/WirelessPhy set CPThresh_ 10.0                  ;#Collision Threshold
#Phy/WirelessPhy set CSThresh_ 5.011872e-12          ;#Carrier Sense Power
#Phy/WirelessPhy set RXThresh_ 5.82587e-09           ;#Receive Power Threshold; calculated under TwoRayGround model by tools from NS2. 
#Phy/WirelessPhy set CSThresh_ 10.00e-12
#Phy/WirelessPhy set RXThresh_ 10.00e-11

# Initialize Global Variables
set ns_		[new Simulator]
set tracefd     [open infra.tr w]
#set tracefd      stderr

#$ns_ use-newtrace
$ns_ trace-all       $tracefd


# set up topography object
set topo       [new Topography]

$topo load_flatgrid $val(x) $val(y)

# Create God
create-god $val(nn)

# Create channel
set chan_1_ [new $val(chan)]

set initial_position_x(0)  0
set initial_position_y(0)  0
set initial_position_z(0)  0

set initial_position_x(1)  0
set initial_position_y(1)  200
set initial_position_z(1)  0

set initial_position_x(2)  100
set initial_position_y(2)  173
set initial_position_z(2)  0

set initial_position_x(3)  173
set initial_position_y(3)  100
set initial_position_z(3)  0

set initial_position_x(4)  200
set initial_position_y(4)  0
set initial_position_z(4)  0





$ns_ node-config -adhocRouting $val(rp) \
		-llType $val(ll) \
		-macType $val(mac) \
		-ifqType $val(ifq) \
		-ifqLen $val(ifqlen) \
		-antType $val(ant) \
		-propType $val(prop) \
		-phyType $val(netif) \
		-topoInstance $topo \
		-agentTrace ON \
		-routerTrace OFF \
		-macTrace ON \
                -phyTrace ON \
		-movementTrace ON \
		-channel $chan_1_


      for {set i 0} {$i < [expr $val(nn)]} {incr i} {
                  set node_($i) [$ns_ node]
		
                $node_($i) random-motion 0              ;# disable random motion
  		set mac_($i) [$node_($i) getMac 0]
 

      		$mac_($i) set RTSThreshold_ 3000
		
		$node_($i) set X_ $initial_position_x($i)
  		$node_($i) set Y_ $initial_position_y($i)      ;# Horizontal arrangement of nodes
  		$node_($i) set Z_ $initial_position_z($i)
		
	}

#Set Node 0 as the APs.


set AP_ADDR1 [$mac_(0) id]
$mac_(0) ap $AP_ADDR1

#set another node as AP

set node_an [$ns_ node]
$node_an random-motion 0 
set mac_an  [$node_an getMac 0]
$mac_an set RTSThreshold_ 3000
$node_an set X_ 420
$node_an set Y_ 0
$node_an set Z_ 0
set AP_ADDR2 [$mac_an id]
$mac_an ap $AP_ADDR2


set node_an2 [$ns_ node]
$node_an2 random-motion 0 
set mac_an2  [$node_an2 getMac 0]
$mac_an2 set RTSThreshold_ 3000
$node_an2 set X_ 425
$node_an2 set Y_ 0
$node_an2 set Z_ 0
set AP_ADDR3 [$mac_an2 id]
$mac_an2 ap $AP_ADDR3

set node_an3 [$ns_ node]
$node_an2 random-motion 0 
set mac_an3  [$node_an3 getMac 0]
$mac_an3 set RTSThreshold_ 3000
$node_an3 set X_ 430
$node_an3 set Y_ 0
$node_an3 set Z_ 0
set AP_ADDR4 [$mac_an3 id]
$mac_an3 ap $AP_ADDR4


for {set i 1} {$i < [expr $val(nn) ]} {incr i} {
	$mac_($i) ScanType PASSIVE	;#ACTIVE
}

#$mac_([expr $val(nn) - 1]) set BeaconInterval_ 0.2
#$ns_ at 1.0 "$mac_(2) ScanType ACTIVE"

Application/Traffic/CBR set packetSize_ 800
Application/Traffic/CBR set rate_ 512Kb




#set node 0 as the UDP agent
for {set i 1} {$i< [expr $val(nn) ]} {incr i} {

set udp_agent($i) [new Agent/UDP]
$ns_ attach-agent $node_(0) $udp_agent($i)


set cbr_app($i)      [new Application/Traffic/CBR]
$cbr_app($i) attach-agent $udp_agent($i)

} 

#set other clients as the receiver	
for {set i 1} {$i < [expr $val(nn)]} {incr i} {


	set base($i) [new Agent/Null]

	$ns_ attach-agent $node_($i) $base($i)
	$ns_ connect      $base($i) $udp_agent($i)
}







for {set i 1} {$i < [expr $val(nn)]} {incr i } {

$ns_ at 4.0  "$cbr_app($i) start"

}




$ns_ at 10.0 "$node_(4) setdest 400.0 30.0 2.5"

$ns_ at 200.0 "stop"
$ns_ at 200.0 "puts \"NS EXITING...\" ; $ns_ halt"

proc stop {} {
    global ns_ tracefd  
    $ns_ flush-trace
    close $tracefd
    exit 0
}

puts "Starting Simulation..."
$ns_ run
