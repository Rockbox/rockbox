#define _PAGE_ Dissecting the Jukebox 6000
#include "head.t"

<p>Taking the Archos apart requires a torx driver and a <i>very</i> small phillips head screwdriver. The phillips screwheads are about 2mm in diameter.

<p>Also see the page dedicated to
<a href="../mods/disassemble.html">disassembling the archos</a>.

<p>
<a href="archos1.jpg"><img src="archos1t.jpg"></a>
<a href="archos2.jpg"><img src="archos2t.jpg"></a>
<a href="archos3.jpg"><img src="archos3t.jpg"></a>
<a href="archos4.jpg"><img src="archos4t.jpg"></a>

<p>The two circuit boards in the Archos are here called the "top" and "bottom" board. They are both populated on both sides.

<h3>Bottom of bottom board</h3>

<p><a href="archos_bottom.jpg"><img src="archos_b1.jpg"></a> (142kB). You will note five ICs in the picture:

<ul>
<li><a href="http://www.in-system.com/200_silicon.html">In-Systems ISD200</a> ATA to USB bridge
<li><a href="http://www.sst.com/products.xhtml/parallel_flash/37/SST37VF020">SST 37VF020</a> 2MB flash ROM
<li><a href="http://www.issiusa.com/pdf/41c16105.pdf">ISSI IS41LV16105</a> 2MB fast page DRAM
<li><a href="http://www.sipex.com/products/pdf/SP690_805ALM.pdf">Sipex SP692ACN</a> Low Power Microprocessor Supervisory with Battery Switch-Over (partly covered with white insulation in the photo)
<li>A standard Motorola AC139 logic IC (text unreadable in the photo)
</ul>

<h3>Top of bottom board</h3>
<p>Removing the bottom board involves bending a couple of metal holders that break very easily. Be careful. 
The board is connected via two pin connectors, one at each end.
<p><a href="archos_bottom2.jpg"><img src="archos_b2.jpg"></a> (211kB). ICs:
<ul>
<li>Archos DCMP3J, most likely an 
<a href="http://www.hitachisemiconductor.com/sic/jsp/japan/eng/products/mpumcu/32bit/superh/sh7032_e.html">SH7034</a>
SH-1 RISC with custom mask rom. (Thanks to Sven Karlsson.)
<li><a href="http://focus.ti.com/docs/prod/folders/print/cd54hc573.html">TI HC573M</a> Latch (appears unlabeled in the photo)
<li><a href="http://www.fairchildsemi.com/pf/74/74LCX245.html">Fairchild LCX245</a> Bidirectional Transceiver
<li>A standard Motorola AC32 logic IC (xor)
</ul>

<h3>Bottom of top board</h3>
<a href="archos_top.jpg"><img src="archos_t1.jpg"></a> (200kB). IC:s:
<ul>
<li><a href="http://www.micronas.com/products/documentation/consumer/mas3507d/index.php">Micronas MAS3507D</a> MPEG-1/2 Layer-2/3 Decoder
<li><a href="http://www.micronas.com/products/documentation/consumer/dac3550a/index.php">Micronas DAC3550A</a> Stereo Audio DAC
</ul>

#include "foot.t"
