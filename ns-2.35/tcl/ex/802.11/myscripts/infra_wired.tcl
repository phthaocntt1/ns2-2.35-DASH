set val(chan)           Channel/WirelessChannel    ;#Channel Type
set val(prop)           Propagation/TwoRayGround   ;# radio-propagation model
set val(netif)          Phy/WirelessPhy            ;# network interface type
set val(mac)            Mac/802_11                 ;# MAC type
set val(ifq)            Queue/DropTail		   ;# interface queue type
set val(ll)             LL                         ;# link layer type
set val(ant)            Antenna/OmniAntenna        ;# antenna model
set val(ifqlen)         50                        ;# max packet in ifq
set val(nn)             5                         ;# number of mobilenodes
set val(rp)             DumbAgent                  ;# routing protocol
set val(x)		600
set val(y)		600

set val(wiredbd)        50Mb
set val(wireddelay)     10ms
set val(wiredQType)     DropTail

Mac/802_11 set dataRate_ 11Mb

#Phy/WirelessPhy set CSThresh_ 10.00e-12
#Phy/WirelessPhy set RXThresh_ 10.00e-11
#Phy/WirelessPhy set Pt_ 0.1
#Phy/WirelessPhy set Pt_ 7.214e-3

# Initialize Global Variables
set ns_		[new Simulator]
set tracefd     [open infra.tr w]
set namtrace    [open out.nam  w]
#$ns_ use-newtrace
$ns_ trace-all       $tracefd
$ns_  namtrace-all-wireless   $namtrace $val(x) $val(y)


# set up topography object
set topo       [new Topography]

$topo load_flatgrid $val(x) $val(y)

# Create God
create-god $val(nn)

# Create channel
set chan_1_ [new $val(chan)]

set initial_position_x(0)  0
set initial_position_y(0)  0

set initial_position_x(1)  0
set initial_position_y(1)  200

set initial_position_x(2)  100
set initial_position_y(2)  173

set initial_position_x(3)  173
set initial_position_y(3)  100

set initial_position_x(4)  200
set initial_position_y(4)  0

# create and connect 5 wired nodes
for {set i 0} {$i < [expr $val(nn)]} {incr i} {

            set wnode_($i) [$ns_ node]
}

$ns_ duplex-link $wnode_(0) $wnode_(1) $val(wiredbd) $val(wireddelay) $val(wiredQType)
$ns_ duplex-link $wnode_(1) $wnode_(2) $val(wiredbd) $val(wireddelay) $val(wiredQType)
$ns_ duplex-link $wnode_(2) $wnode_(3) $val(wiredbd) $val(wireddelay) $val(wiredQType)
$ns_ duplex-link $wnode_(3) $wnode_(4) $val(wiredbd) $val(wireddelay) $val(wiredQType)


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
		-movementTrace ON \
		-channel $chan_1_


      for {set i 0} {$i < [expr $val(nn)]} {incr i} {
                  set node_($i) [$ns_ node]
		
                $node_($i) random-motion 0              ;# disable random motion
  		set mac_($i) [$node_($i) getMac 0]
 

      		$mac_($i) set RTSThreshold_ 3000
		
		$node_($i) set X_ $initial_position_x($i)
  		$node_($i) set Y_ $initial_position_y($i)      ;# Horizontal arrangement of nodes
  		$node_($i) set Z_ 0.0
		
	}

#Set Node 0 as the APs.


set AP_ADDR1 [$mac_(0) id]
$mac_(0) ap $AP_ADDR1


#$mac_([expr $val(nn) - 1]) set BeaconInterval_ 0.2


$ns_ duplex-link $node_(0) $wnode_(4) $val(wiredbd) $val(wireddelay) $val(wiredQType)



for {set i 1} {$i < [expr $val(nn) ]} {incr i} {
	$mac_($i) ScanType PASSIVE	;#ACTIVE
}


#$ns_ at 1.0 "$mac_(2) ScanType ACTIVE"

Application/Traffic/CBR set packetSize_ 800
Application/Traffic/CBR set rate_ 512Kb




#set wired node 0 as the UDP agent
for {set i 1} {$i< [expr $val(nn) ]} {incr i} {

set udp_agent($i) [new Agent/UDP]
$ns_ attach-agent $wnode_(0) $udp_agent($i)


set cbr_app($i)      [new Application/Traffic/CBR]
$cbr_app($i) attach-agent $udp_agent($i)

} 

#set other clients as the receiver	
for {set i 1} {$i < [expr $val(nn) ]} {incr i} {


	set base($i) [new Agent/Null]

	$ns_ attach-agent $node_($i) $base($i)
	$ns_ connect      $base($i) $udp_agent($i)
}






for {set i 1} {$i < [expr $val(nn)]} {incr i } {


$ns_ at 4.0 "$cbr_app($i) start"

}



#$ns_ at 10.0 "$node_(4) setdest 300.0 1.0 30.0"

$ns_ at 20.0 "stop"
$ns_ at 20.0 "puts \"NS EXITING...\" ; $ns_ halt"

proc stop {} {
    global ns_ tracefd  namtrace
    $ns_ flush-trace
    close $tracefd
    close $namtrace
    exit 0
}

puts "Starting Simulation..."
$ns_ run
