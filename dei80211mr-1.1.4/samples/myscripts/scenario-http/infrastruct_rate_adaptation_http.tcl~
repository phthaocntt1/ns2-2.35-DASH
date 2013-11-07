#
# Copyright (c) 2007 Regents of the SIGNET lab, University of Padova.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University of Padova (SIGNET lab) nor the 
#    names of its contributors may be used to endorse or promote products 
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

 
########################################
# Command-line parameters
########################################


if {$argc == 0} {

    # default parameters if none passed on the command line
    set opt(nn) 8
    set opt(xdist) 200
    set opt(raname) snr
    set opt(run) 1

} elseif {$argc != 4 } {
    puts " invalid argc ($argc)"
    puts " usage: $argv0 numnodes xdist RateAdapter replicationnumber"
    exit
} else {

    set opt(nn)      [lindex $argv 0]       
    set opt(xdist)   [lindex $argv 1]
    set opt(raname)  [lindex $argv 2]
    set opt(run)     [lindex $argv 3]

}


if { $opt(raname) == "snr" } {
    set opt(ra) "SNR"
} elseif { $opt(raname) == "arf" } {
    set opt(ra) "ARF"    
} else {
    puts "unknown rate adaptation scheme \"$opt(raname)\""
    exit
} 






########################################
# Scenario Configuration
########################################
#set video description file
set video_file "video-list.txt"
set requested_video "mimuroto"

# duration of each transmission
set opt(duration)     5000

# starting time of each transmission
set opt(startmin)     1
set opt(startmax)     1.25

set opt(resultdir) "."
set opt(tracedir) "."
set machine_name [exec uname -n]
set opt(fstr)        ${argv0}_${opt(nn)}_${opt(raname)}.${machine_name}
set opt(resultfname) "${opt(resultdir)}/stats_${opt(fstr)}.log"
set opt(tracefile)   "${opt(tracedir)}/${argv0}.tr"

########################################
# Module Libraries
########################################


# The following lines must be before loading libraries
# and before instantiating the ns Simulator
#
#remove-all-packet-headers



set system_type [exec uname -s]

if {[string match "CYGWIN*" "$system_type"] == 1} {
    load ../../../src/.libs/cygdei80211mr-0.dll
} else {
#if {string match Linux* $system_type} 
    load ../../../src/.libs/libdei80211mr.so
} 



########################################
# Simulator instance
########################################


set ns [new Simulator]


########################################
# Random Number Generators
########################################

global defaultRNG
set startrng [new RNG]

set rvstart [new RandomVariable/Uniform]
$rvstart set min_ $opt(startmin)
$rvstart set max_ $opt(startmax)
$rvstart use-rng $startrng

# seed random number generator according to replication number
for {set j 1} {$j < $opt(run)} {incr j} {
    $defaultRNG next-substream
    $startrng next-substream
}




########################################
# Some procedures 
########################################

proc finish {} {
    global ns tf opt 
    puts "done!"
    $ns flush-trace
    close $tf
    print_stats
    $ns halt 
    puts " "
    puts "Tracefile     : $opt(tracefile)"
    puts "Results file  : $opt(resultfname)"
    
    #puts "node moving from 0m at [expr int($opt(startmoving))]s to $opt(xdist)m at [expr int($opt(endmoving))]s, [expr $opt(nn) - 1] interferers, $opt(ra) scheme"    
}



proc print_stats  {} {
    global fh_cbr opt me_mac me_phy peerstats 

    set resultsFilePtr [open $opt(resultfname) w]

    #for {set id 1} {$id <= 1 } {incr id} 
    for {set id 1} {$id <= $opt(nn) } {incr id}  {


	set snr [$me_mac($id) getAPSnr ]
	set peerstatsstr [$peerstats getPeerStats $id 0]
	set mestatsstr   [$me_mac($id) getMacCounters]
	set logstr "$opt(run) $opt(nn) $opt(duration) $id $snr \n$mestatsstr \n$peerstatsstr "

	#puts $logstr
 	puts $resultsFilePtr $logstr

    }

    close $resultsFilePtr

}


Node/MobileNode instproc getIfq { param0} {
    $self instvar ifq_    
    return $ifq_($param0) 
}

Node/MobileNode instproc getPhy { param0} {
    $self instvar netif_    
    return $netif_($param0) 
}



########################################
# Override Default Module Configuration
########################################

PeerStatsDB set VerboseCounters_ 1
PeerStatsDB/Static set debug_ 0

Mac/802_11/Multirate set VerboseCounters_ 1


Mac/802_11 set RTSThreshold_ 50000
Mac/802_11 set ShortRetryLimit_ 8
Mac/802_11 set LongRetryLimit_  5
Mac/802_11/Multirate set useShortPreamble_ true
Mac/802_11/Multirate set gSyncInterval_ 0.000005
Mac/802_11/Multirate set bSyncInterval_ 0.00001
Mac/802_11 set CWMin_         32
Mac/802_11 set CWMax_         1024

#for Access Point, transmit power set as 100mW
#for clients, transmit power set as 20 mW,or 13 dBm,or -17dB
#free space path loss (FSPL)
#fspl=20log10(d)+40.19
#100m: 80.19 dB , signal -97.19 dB
#50m:  74.19 dB , signal -91.19 dB,to reach 23dB threashold, noise should be -114.19 dB,or -84.19 dBm
Phy/WirelessPhy set Pt_ 0.02
Phy/WirelessPhy set freq_ 2.437e9
Phy/WirelessPhy set L_ 1.0


Queue/DropTail/PriQueue set Prefer_Routing_Protocols    1
Queue/DropTail/PriQueue set size_ 50


# about -91.6 dBm
set noisePower 7e-13
Phy/WirelessPhy set CSTresh_ [expr $noisePower * 1.1]


# a large enough value
Channel/WirelessChannel/PowerAware set distInterference_ 300



###############################
# Global allocations
###############################



set tf [open $opt(tracefile) w]
$ns trace-all $tf

set channel   [new Channel/WirelessChannel/PowerAware]

set topo      [new Topography]
$topo load_flatgrid [expr $opt(xdist) + 1] [expr $opt(xdist) + 1]

set god       [create-god [expr $opt(nn) + 1]]

set per       [new PER]
$per set noise_ $noisePower
$per loadPERTable80211gTrivellato

set peerstats [new PeerStatsDB/Static]
$peerstats numpeers [expr $opt(nn) + 1]




###############################
# Node Configuration
###############################


$ns node-config -adhocRouting DumbAgent \
            -llType  DashLL   \
            -macType Mac/802_11/Multirate \
            -ifqType Queue/DropTail/DashPriQueue \
            -ifqLen 500 \
            -antType Antenna/OmniAntenna \
            -propType Propagation/FreeSpace/PowerAware \
	    -phyType Phy/WirelessPhy/PowerAware \
	    -topoInstance $topo \
	    -agentTrace ON \
            -routerTrace OFF \
	    -macTrace ON \
            -ifqTrace ON \
            -phyTrace ON \
	    -channel $channel



#######################################
#  Create Base Station (Access Point) #
#######################################


set bs_node        [$ns node]

set bs_mac [$bs_node getMac 0]
set bs_ifq [$bs_node getIfq 0]
set bs_phy [$bs_node getPhy 0]

$bs_mac nodes [expr $opt(nn) + 1]
$bs_mac  basicMode_ Mode6Mb
$bs_mac  dataMode_ Mode54Mb
$bs_mac  per $per
$bs_mac  PeerStatsDB $peerstats
set bs_pp  [new PowerProfile]
$bs_mac  powerProfile $bs_pp
$bs_phy   powerProfile $bs_pp

set bs_macaddr [$bs_mac id]
$bs_mac     bss_id $bs_macaddr

$bs_node random-motion 0
$bs_node set X_ 0
$bs_node set Y_ 0
$bs_node set Z_ 0



###############################
#  Create Mobile Equipments 
###############################
set init_pos_x(1) 50
set init_pos_y(1) 0

set init_pos_x(2) 43.5
set init_pos_y(2) 25

set init_pos_x(3) 25
set init_pos_y(3) 43.5

set init_pos_x(4) 0
set init_pos_y(4) 50

set init_pos_x(5) 0
set init_pos_y(5) -50

set init_pos_x(6) -50
set init_pos_y(6) 0

set init_pos_x(7) -43.5
set init_pos_y(7) -25

set init_pos_x(8) -25
set init_pos_y(8) -43.5

for {set id 1} {$id <= $opt(nn)} {incr id} {

    set me_node($id)   [$ns node]

    set me_mac($id) [$me_node($id) getMac 0]
    set me_ifq($id) [$me_node($id) getIfq 0]
    set me_phy($id) [$me_node($id) getPhy 0]
    
    $me_mac($id) nodes [expr $opt(nn) + 1]
    $me_mac($id)   basicMode_ Mode6Mb
    $me_mac($id)   dataMode_ Mode54Mb
    $me_mac($id)   per $per
    $me_mac($id)   PeerStatsDB $peerstats
    set me_pp($id)   [new PowerProfile]
    $me_mac($id)   powerProfile $me_pp($id) 
    $me_phy($id)    powerProfile $me_pp($id)

    #power of clients' radio
    $me_phy($id)  set Pt_ 0.02
    $me_mac($id)  bss_id $bs_macaddr

    $me_node($id) set X_ $init_pos_x($id)
    $me_node($id) set Y_ $init_pos_y($id)
    $me_node($id) set Z_ 0
    $me_node($id) random-motion 0


    # Create Protocol Stack 
     

    # we set only one dash server and client, all others are common UDP traffic
     if {$id == 1} {
    
     # an http agent is basically a fulltcp agent, but knows how to deal with packet with contents
     set bs_tran($id)   [new Agent/TCP/FullTcp/HTTP]
     $bs_tran($id) set fid_ $id
     $bs_tran($id) set segsize_ 1460 #default packet size
     $bs_tran($id) set window_ 64  #maximum congestion window size (pkts)   

    # attach the agent to base station
    $ns attach-agent   $bs_node $bs_tran($id)


    set me_cbr($id)    [new Application/HTTP/DashServer]
    
     #attach video file
    $me_cbr($id)    read-video-list $video_file
    $me_cbr($id)    attach-agent $bs_tran($id)


    # For http agent at BS for dash server, we also need a corresponding one at the MS for dash client

    
    set me_tran($id)   [new Agent/TCP/FullTcp/HTTP]
    $me_tran($id) set fid_ $id
    $me_tran($id) set segsize_ 1460
    $me_tran($id) set window_ 64

    # attach the agent to base station
    $ns attach-agent $me_node($id) $me_tran($id)


    #set dash_client($id) [new Application/HTTP/DashClient]
    #set dash_client($id) [new Application/HTTP/AndroidDashClient]
    #set dash_client($id) [new Application/HTTP/VLCHLSClient]
    #set dash_client($id) [new Application/HTTP/VLCDashClient]
    set dash_client($id) [new Application/HTTP/AIMDDashClient]   

    $dash_client($id) request $requested_video

    $dash_client($id) attach-agent $me_tran($id)

    $ns connect $bs_tran($id) $me_tran($id)
    
    #set the server at the listen state for incoming request from client
    $bs_tran($id) listen

    }  else {  
      
      # configuration for other nodes with UDP traffic
      set bs_tran($id) [new Agent/UDP]
      
      #attach the agent to base station
      $ns attach-agent $bs_node $bs_tran($id)
       
      set me_cbr($id)    [new Application/Traffic/CBR]
      $me_cbr($id)    attach-agent $bs_tran($id) 

      # For each agent at the BS  we also need a corresponding one at the MS

      set me_tran($id)   [new Agent/Null]
      $ns attach-agent $me_node($id) $me_tran($id)

      # setup socket connection between ME and BS
      $ns connect $bs_tran($id) $me_tran($id) 

    }

    

    # setup socket connection between ME and BS
    $ns connect $bs_tran($id) $me_tran($id) 
    $me_tran($id) listen


    # schedule data transfer
   # set cbr_start($id) [expr $rvstart value]

    if {$id ==1 } {

    set cbr_start($id) 0

    
    } else {

    set cbr_start($id) [expr 30*$id+90]
    }

    
    if {$id<7} {
    set cbr_stop($id) [expr $cbr_start($id) + $opt(duration)]
    } else {
    set cbr_stop($id) [expr $cbr_start($id) + 300]
    }
    

    

    puts "node $id starts at $cbr_start($id) and stops at $cbr_stop($id)"


    # set traffic type

    #dash no longder need traffic type configuration
    if {$id == 1} {

	# Test node (id==1) generates CBR traffic station using PHY rate adaptation


	#$me_cbr($id) set packetSize_      1000
	#$me_cbr($id) set rate_            1200000  #1.2Mb/s
	#$me_cbr($id) set random_          0

 	#set speed($id) [expr int ( ($opt(xdist) + 0.0-$init_pos_x(1)) / ($opt(duration)/2.0) )]
         set speed($id) 0.5
 	puts "Speed of test node: $speed($id) m/s"

	set opt(startmoving) [expr $cbr_start($id) + 100]
	set opt(endmoving)   [expr $cbr_start($id) + $opt(duration) ]

 	#$ns at $opt(startmoving) "$me_node($id) setdest 70.0 0.25 $speed($id)"
      
       # $ns at 200 "$me_node($id) setdest 40 0.25 $speed($id)"
	#set pktsize [expr [$me_cbr($id) set packetSize_] + 0.0]


        #set rate adaptation scheme
	#set me_ra($id) [new RateAdapter/$opt(ra)]

	#if { $opt(ra) == "SNR" } {
	 #   $me_ra($id) set pktsize_ $pktsize

	#} 

	#$me_ra($id) attach2mac $me_mac($id)
	#$me_ra($id) use80211g
	#$me_ra($id) setmodeatindex 0


    } else {

	# All other nodes (id!=1) are in saturation and use a fixed PHY rate
	# Saturated station

	$me_cbr($id) set packetSize_      1500
	#$me_cbr($id) set rate_            [ expr 14000000 / ( $opt(nn) - 1) ]
        $me_cbr($id) set rate_            3000000


	# These are needed to match Bianchi Model
	$me_mac($id) set ShortRetryLimit_ 100
	$me_mac($id) set LongRetryLimit_  100

    }



    if {$id == 1} {
    
   $ns at $cbr_start($id) "$dash_client($id) start"
   $ns at $cbr_stop($id)  "$dash_client($id) stop"

   }  else {

    $ns at $cbr_start($id)  "$me_cbr($id) start"
    $ns at $cbr_stop($id)   "$me_cbr($id) stop"

   }



}




###############################
#  Start Simulation
###############################

puts -nonewline "Simulating"


for {set i 1} {$i < 40} {incr i} {
    $ns at [expr $opt(startmax) + (($opt(duration) * $i)/ 40.0) ] "puts -nonewline . ; flush stdout"
}
$ns at [expr $opt(startmax) + $opt(duration) + 5] "finish"
$ns run
