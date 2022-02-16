# Main Tcl script for Generator

### Setup global variables and options ###

global debug, sharedir, savename, nextdump, name, copyright, scale, smooth
global layerB, layerBp, layerA, layerAp, layerH, layerS, layerSp, vdpsimple
global layerW, layerWp
global skip
set sharedir {/usr/local/share/generator}
set savename {}
set tk_strictMotif 1
set nextdump 1
set name {}
set copyright {}

### Main window definition ###

# Set window attributes: title and make unresizable
wm title . Generator
wm resizable . 0 0

# Split main window into two frames, the top one is the menu bar
frame .bar -relief raised -bd 2
frame .main -width 320 -height 224 -bg black
pack .bar -side top -fill x
pack .main -side top -side top -anchor center

# Create left-side menu names on the menu bar
menubutton .bar.file -text File -underline 0 -menu .bar.file.menu
menubutton .bar.open -text Open -underline 0 -menu .bar.open.menu
menubutton .bar.display -text Display -underline 0 -menu .bar.display.menu
menubutton .bar.game -text Game -underline 0 -menu .bar.game.menu
if {$debug == 1} {
    menubutton .bar.debug -text Debug -underline 0 -menu .bar.debug.menu
    pack .bar.file .bar.open .bar.display .bar.game .bar.debug -side left
} else {
    pack .bar.file .bar.open .bar.display .bar.game -side left
}

# Create right-side menu names on the menu bar
menubutton .bar.help -text Help -underline 0 -menu .bar.help.menu
pack .bar.help -side right

# Create some key bindings
bind . <Control-c> {destroy .}
bind . q {destroy .}

# Define File menu
menu .bar.file.menu -tearoff true
.bar.file.menu add command -label {Open image} -command {File_open 0}
#.bar.file.menu add command -label {Open saved position} -command {File_open 1}
#.bar.file.menu add command -label Save -command {File_save $savename}
#.bar.file.menu add command -label "Save as" -command File_saveas
.bar.file.menu add separator
.bar.file.menu add command -label Quit -command {destroy .}

# Define Open menu
menu .bar.open.menu -tearoff true
.bar.open.menu add command -label {Image information} -command {Info_open}

# Define Display menu
menu .bar.display.menu -tearoff true
.bar.display.menu add radiobutton -variable myscale -label "100%" -value 1 \
	-command "Display_set 1"
.bar.display.menu add radiobutton -variable myscale -label "200%" -value 2 \
	-command "Display_set 2"
.bar.display.menu add radiobutton -variable myscale -label "200% smooth" \
	-value 3 -command "Display_set 3"
.bar.display.menu add separator
.bar.display.menu add radiobutton -variable vdpsimple \
	-label "Complex (slow) VDP" \
	-value 0 -command "Gen_Change"
.bar.display.menu add radiobutton -variable vdpsimple \
	-label "Simple (fast) VDP" \
	-value 1 -command "Gen_Change"
.bar.display.menu add separator
.bar.display.menu add checkbutton -variable layerB  -label "Layer B" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add checkbutton -variable layerA  -label "Layer A" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add checkbutton -variable layerW  -label "Window" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add checkbutton -variable layerH  -label "Shadow" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add checkbutton -variable layerS  -label "Sprites" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add checkbutton -variable layerBp -label "Layer B pri" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add checkbutton -variable layerAp -label "Layer A pri" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add checkbutton -variable layerWp -label "Window pri" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add checkbutton -variable layerSp -label "Sprites pri" \
	-onvalue 1 -command "Gen_Change"
.bar.display.menu add separator
.bar.display.menu add radiobutton -variable skip -label "All frames" \
	-value 1 -command "Gen_Change"
.bar.display.menu add radiobutton -variable skip -label "Every other frame" \
	-value 2 -command "Gen_Change"
.bar.display.menu add radiobutton -variable skip -label "Every 3 frames" \
	-value 3 -command "Gen_Change"
.bar.display.menu add radiobutton -variable skip -label "Every 4 frames" \
	-value 4 -command "Gen_Change"
.bar.display.menu add radiobutton -variable skip -label "Every 5 frames" \
	-value 5 -command "Gen_Change"

# Define Game menu
menu .bar.game.menu -tearoff true
.bar.game.menu add radiobutton -variable state -label "Stop" -value 0 \
	-command "Gen_Change"
.bar.game.menu add radiobutton -variable state -label "Pause" -value 1 \
	-command "Gen_Change"
.bar.game.menu add radiobutton -variable state -label "Play" -value 2 \
	-command "Gen_Change"
.bar.game.menu add separator
.bar.game.menu add radiobutton -variable loglevel -label {Log quiet} \
	-value 0 -command "Gen_Change"
.bar.game.menu add radiobutton -variable loglevel -label {Log critical} \
	-value 1 -command "Gen_Change"
.bar.game.menu add radiobutton -variable loglevel -label {Log normal} \
	-value 2 -command "Gen_Change"
.bar.game.menu add radiobutton -variable loglevel -label {Log verbose} \
	-value 3 -command "Gen_Change"
.bar.game.menu add radiobutton -variable loglevel -label {Log user} \
	-value 4 -command "Gen_Change"
.bar.game.menu add radiobutton -variable loglevel -label {Log debug1} \
	-value 5 -command "Gen_Change"

# Set default values for Display menu
set myscale 1
set layerB 1
set layerBp 1
set layerA 1 
set layerAp 1 
set layerW 1 
set layerWp 1 
set layerH 1
set layerS 1
set layerSp 1
set skip 3
set vdpsimple 1
set state 0
set loglevel 2

# Define Help menu
menu .bar.help.menu -tearoff true
.bar.help.menu add command -label Generator -command "Help_show generator.hlp"
.bar.help.menu add command -label Genesis -command "Help_show genesis.hlp"
.bar.help.menu add separator
.bar.help.menu add command -label Copyright -command "Help_show copyright.hlp"

Gen_Change

# Define Debug menu
if {$debug} {
    menu .bar.debug.menu -tearoff true
    .bar.debug.menu add command -label {Disassemble ROM} -command \
	    {Debug_dump 0 0 0}
    .bar.debug.menu add command -label {Disassemble RAM} -command \
	    {Debug_dump 1 0 0}
    .bar.debug.menu add command -label {Disassemble VRAM} -command \
	    {Debug_dump 2 0 0}
    .bar.debug.menu add command -label {Disassemble CRAM} -command \
	    {Debug_dump 3 0 0}
    .bar.debug.menu add command -label {Disassemble VSRAM} -command \
	    {Debug_dump 4 0 0}
    .bar.debug.menu add command -label {Disassemble SRAM} -command \
	    {Debug_dump 5 0 0}
    .bar.debug.menu add command -label {VDP register dump} -command {Gen_Regs}
    .bar.debug.menu add command -label {Describe VDP} \
	    -command {Gen_VDPDescribe}
    .bar.debug.menu add command -label {Profile clear} \
	    -command {Gen_ProfileClr}
    .bar.debug.menu add command -label {Profile dump} \
	    -command {Gen_ProfileDump}
    .bar.debug.menu add command -label {Sprite list dump} \
	    -command {Gen_SpriteList}
    .bar.debug.menu add command -label {Registers} -command {Regs_open}
    .bar.debug.menu add separator
    .bar.debug.menu add command -label {Reset z80} -command {Gen_Reset 1}
}

Gen_Initialised

### File menu support functions ###

proc File_open {savetype} {
    set types {
	{{Generator Save Game files} {.gsg} }
	{{All files} * }
    }
    set filename [tk_getOpenFile -parent . -defaultextension gsg \
	    -title [expr { $savetype ? {Open saved position} : \
	    {Open image} }] -filetypes $types]
    if {[string compare $filename {}] == 0} {
	return
    }
    Gen_Load $savetype $filename
}

proc File_save {filename} {
    global savename

    if {[string compare $filename {}] == 0} {
	File_saveas
	return
    }
    set savename $filename
    puts $filename
}

proc File_saveas {} {
    set filename [tk_getSaveFile -parent . -initialfile {savegame.gsg} \
	    -defaultextension gsg -title {Save position as}]
    if {[string compare $filename {}] == 0} {
	return
    }
    File_save $filename
}

### Display menu support functions ###

proc Display_set {myscale} {
    global smooth scale
    set smooth 0
    if {$myscale == 1} {
	set scale 1
	.main configure -width 320 -height [expr 224]
    } else {
	set scale 2
	if {$myscale == 2} {
	    .main configure -width 640 -height [expr 448]
	} else {
	    set smooth 1
	    .main configure -width 640 -height [expr 448]
	}
    }
    Gen_Change
}

### Help menu support functions ###

proc Help_show file {
    global sharedir
    if ![winfo exists .help] {
	toplevel .help
	text .help.text -relief raised -bd 2 -yscrollcommand ".help.scroll set"
	scrollbar .help.scroll -command ".help.text yview"
	pack .help.scroll -side right -fill y
	pack .help.text -side top -fill both -expand true
    }
    .help.text configure -state normal
    .help.text delete 1.0 end
    set f [open $sharedir/$file]
    while {![eof $f]} {
	.help.text insert end [read $f 1000]
    }
    close $f
    .help.text configure -state disabled
}

### Info menu support functions ###

proc Info_open {} {
    global i_console i_copyright i_name_domestic i_name_overseas
    global i_prodtype i_version i_checksum
    if ![winfo exists .info] {
	toplevel .info
	label .info.lconsole -text "Console" -anchor w
        entry .info.console -width 32 -textvariable i_console
	grid .info.lconsole .info.console -sticky news
	label .info.lcopyright -text Copyright -anchor w
        entry .info.copyright -width 32 -textvariable i_copyright
	grid .info.lcopyright .info.copyright -sticky news
	label .info.ldomestic -text "Domestic name" -anchor w
        entry .info.domestic -width 32 -textvariable i_name_domestic
	grid .info.ldomestic .info.domestic -sticky news
	label .info.loverseas -text "Overseas name" -anchor w
        entry .info.overseas -width 32 -textvariable i_name_overseas
	grid .info.loverseas .info.overseas -sticky news
	label .info.lprodtype -text "Product Type" -anchor w
        entry .info.prodtype -width 32 -textvariable i_prodtype
	grid .info.lprodtype .info.prodtype -sticky news
	label .info.lversion -text "Version" -anchor w
        entry .info.version -width 32 -textvariable i_version
	grid .info.lversion .info.version -sticky news
	label .info.lchecksum -text "Checksum" -anchor w
        entry .info.checksum -width 32 -textvariable i_checksum
	grid .info.lchecksum .info.checksum -sticky news
	wm resizable .info 0 0
    }
}

### Regs menu support functions ###

proc Regs_open {} {
    global regs.d0 regs.d1 regs.d2 regs.d3 regs.d4 regs.d5 regs.d6 regs.d7
    global regs.a0 regs.a1 regs.a2 regs.a3 regs.a4 regs.a5 regs.a6 regs.a7
    global regs.s regs.x regs.n regs.z regs.v regs.c regs.stop
    global regs.clocks regs.frames
    if ![winfo exists .regs] {
	toplevel .regs
	label .regs.d0txt -text "D0" -relief flat
	label .regs.d1txt -text "D1" -relief flat
	label .regs.d2txt -text "D2" -relief flat
	label .regs.d3txt -text "D3" -relief flat
	label .regs.d4txt -text "D4" -relief flat
	label .regs.d5txt -text "D5" -relief flat
	label .regs.d6txt -text "D6" -relief flat
	label .regs.d7txt -text "D7" -relief flat
	grid .regs.d0txt .regs.d1txt .regs.d2txt .regs.d3txt .regs.d4txt .regs.d5txt .regs.d6txt .regs.d7txt -sticky news
	button .regs.d0 -textvariable regs.d0 -width 9 \
		-command {Regs_dump ${regs.d0}}
	button .regs.d1 -textvariable regs.d1 -width 9 \
		-command {Regs_dump ${regs.d1}}
	button .regs.d2 -textvariable regs.d2 -width 9 \
		-command {Regs_dump ${regs.d2}}
	button .regs.d3 -textvariable regs.d3 -width 9 \
		-command {Regs_dump ${regs.d3}}
	button .regs.d4 -textvariable regs.d4 -width 9 \
		-command {Regs_dump ${regs.d4}}
	button .regs.d5 -textvariable regs.d5 -width 9 \
		-command {Regs_dump ${regs.d5}}
	button .regs.d6 -textvariable regs.d6 -width 9 \
		-command {Regs_dump ${regs.d6}}
	button .regs.d7 -textvariable regs.d7 -width 9 \
		-command {Regs_dump ${regs.d7}}
	grid .regs.d0 .regs.d1 .regs.d2 .regs.d3 .regs.d4 .regs.d5 .regs.d6 .regs.d7 -sticky news
	label .regs.a0txt -text "A0" -relief flat
	label .regs.a1txt -text "A1" -relief flat
	label .regs.a2txt -text "A2" -relief flat
	label .regs.a3txt -text "A3" -relief flat
	label .regs.a4txt -text "A4" -relief flat
	label .regs.a5txt -text "A5" -relief flat
	label .regs.a6txt -text "A6" -relief flat
	label .regs.a7txt -text "A7" -relief flat
	grid .regs.a0txt .regs.a1txt .regs.a2txt .regs.a3txt .regs.a4txt .regs.a5txt .regs.a6txt .regs.a7txt -sticky news
	button .regs.a0 -textvariable regs.a0 -width 9 \
		-command {Regs_dump ${regs.a0}}
	button .regs.a1 -textvariable regs.a1 -width 9 \
		-command {Regs_dump ${regs.a1}}
	button .regs.a2 -textvariable regs.a2 -width 9 \
		-command {Regs_dump ${regs.a2}}
	button .regs.a3 -textvariable regs.a3 -width 9 \
		-command {Regs_dump ${regs.a3}}
	button .regs.a4 -textvariable regs.a4 -width 9 \
		-command {Regs_dump ${regs.a4}}
	button .regs.a5 -textvariable regs.a5 -width 9 \
		-command {Regs_dump ${regs.a5}}
	button .regs.a6 -textvariable regs.a6 -width 9 \
		-command {Regs_dump ${regs.a6}}
	button .regs.a7 -textvariable regs.a7 -width 9 \
		-command {Regs_dump ${regs.a7}}
	grid .regs.a0 .regs.a1 .regs.a2 .regs.a3 .regs.a4 .regs.a5 .regs.a6 .regs.a7 -sticky news
	label .regs.sptxt -text "SP" -relief flat
	label .regs.srtxt -text "SR" -relief flat
	label .regs.pctxt -text "PC" -relief flat
	label .regs.stoptxt -text "Stop" -relief flat
	label .regs.clockstxt -text "Clocks" -relief flat
	grid .regs.sptxt .regs.srtxt .regs.pctxt x x x .regs.stoptxt .regs.clockstxt -sticky news
	label .regs.sr -textvariable regs.sr -width 9 -relief groove
	button .regs.pc -textvariable regs.pc -width 9 \
		-command {Regs_dump ${regs.pc}}
	label .regs.clocks -textvariable regs.clocks -width 9 -relief groove
	button .regs.sp -textvariable regs.sp -width 9 \
		-command {Regs_dump ${regs.sp}}
	button .regs.step -text "Step" -command {Gen_Step}
	button .regs.cont -text "Cont" -command {Gen_Cont}
	button .regs.framestep -text "Frame step" -command {Gen_FrameStep}
	entry .regs.stop -textvariable regs.stop -width 9
	grid .regs.sp .regs.sr .regs.pc .regs.step .regs.cont .regs.framestep .regs.stop .regs.clocks -sticky news
	checkbutton .regs.s -variable regs.s -text "S" -onvalue 1
	checkbutton .regs.x -variable regs.x -text "X" -onvalue 1
	checkbutton .regs.n -variable regs.n -text "N" -onvalue 1
	checkbutton .regs.z -variable regs.z -text "Z" -onvalue 1
	checkbutton .regs.v -variable regs.v -text "V" -onvalue 1
	checkbutton .regs.c -variable regs.c -text "C" -onvalue 1
	label .regs.frames -textvariable regs.frames -width 9 -relief groove
	grid .regs.s .regs.x .regs.n .regs.z .regs.c .regs.v x .regs.frames -sticky news
	wm resizable .regs 0 0
    }
}

proc Regs_dump {addr} {
    Debug_dump 0 0 0x$addr
}

### Debug menu support functions ###

proc Debug_dump {memtype dumptype offset} {
    global nextdump
    set dumpname .dump$nextdump
    toplevel $dumpname
    text $dumpname.main -relief raised
    scrollbar $dumpname.bar -command "Gen_Dump $dumpname yview"
    pack $dumpname.bar -fill y -side right
    pack $dumpname.main -expand true -fill both
    global $dumpname.main $dumpname.offset $dumpname.memtype $dumpname.dumptype
    global $dumpname.lines
    set $dumpname.offset $offset
    set $dumpname.memtype $memtype
    set $dumpname.dumptype $dumptype
    set $dumpname.lines 0
    $dumpname.bar set 0 1
    tkwait visibility $dumpname
    $dumpname.main insert end "..."
    Gen_Dump $dumpname redraw
    bind $dumpname <Configure> "Gen_Dump $dumpname redraw"
    set nextdump [expr {$nextdump+1}]
}

### Message window definition ###

proc showmessage {msg} {
    toplevel .message
    message .message.text -relief raised -bd 2 -justify center \
	    -text $msg -width 128
    pack .message.text -side top -fill both -expand true
    button .message.ok -text "OK" -command {destroy .message}
    pack .message.ok -side bottom
    wm minsize .message 192 128
    wm title .message "Message from Generator"
    tkwait visibility .message
    grab set .message
    tkwait window .message
}
