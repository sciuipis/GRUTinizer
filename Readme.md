GRUTinizer
==========

A generic unpacker and analysis package for gamma-ray spectroscopy.

The doxygen documentation can be found [here](https://pcbend.github.io/GRUTinizer).

Tutorial for July RCNP Test Experiment 2016


<a name="top" />

[**HOME**](Home) > [**INTERACTIVE ANALYSIS**](Interactive-Analysis)

#The GROOT library and you.

In addition to the many libraries in TGRUTAnalysis to make analysis easier, GRUTinizer also takes control of some of the behind the scene functions of ROOT to make gamma-ray analysis a bit easier. 

To take advantage of these features all one has to do is start GRUTinizer!  They are implemented when examining and natural root classes, whether the where draw from a tree, made fresh or load from a file.
 
  * **GlobalFunctions** <br>
    
  * **GCanvas** <br>  The most notable difference, is the replacement of the TCanvas with GCanvas.  This replacement is done naturally - no changes from either existing ROOT scripts or GRUTinizer scripts are needed to take advantage of the GCanvas default behavior.  

## Universal Commands

| Key | Action |
|:-----|:------|
| **F2** | Show/Hide Editor Tab |

## TH1 Commands
|  Key  | Action |
|:------|:------|
|        | All normal root commands/interactions.|
| **m**  | Toggle on/off marker mode; when on, the histogram will remember and display the last four clicks as marks on the histogram.|
| **p**  | If the 1d hist was made using the global ProjectionX/ProjectionY; gating the original 2D matrix this histogram came from is possible by placing markers around the gate and pressing p.  The gates spectra is immediately drawn. |
| **B** | Cycle through types of automatic background subtraction used when projecting with **p**.  Current types include: No subtraction, Fraction of the total, subtract gate from the 3rd marker (gate size set to the distance between marker 1 and 2). |
| **b** | Set the background, how it is set depends on **B**.|
| **n**  | Remove all markers / functions drawn on the histogram (not gates!).|
| **e**  | Expand the x-axis range between the last two markers.|
| **E**  | Bring up dialogue box used to set desired x-axis range.|
| **o**  | Unzoom the entire histogram.|
| **Arrow Left/Right** |  When zoomed in, mover the display region to the left/right by one half of the region currently displayed.|
| **Arrow Up/Down** |  Quickly display the next histogram stored in memory, especially useful when gating to go back and forth between gates and the total projection. (currently only available in GH1D)  |
| **f**  | Ryan D's TPeak Fit (proper skewd gaus for gamma-rays with automatic bg) with minimum output. |  
| **g**  | Gaus fit with linear background, displays results of the fit **RESULTS STILL NEED TO BE VERIFIED** |
| **i**  | Raw integral of counts between the two markers |
| **s**  | Show peak values. |
| **S**  | Remove peak values. |
| **l**  | Toggle y-axis linear. |


## GH2I Commands
|  Key  | Action |
|:------|:------|
|        | All normal root commands/interactions.|
| middle-click | Select the current pad, current pad is outlined by a red border. |
| **e**  | Expand the x-axis range between the two markers.|
| **g**  | Create a TCuG on the canvas, name scheme is _cut# where # is tracked from the start of the program.|
| **o**  | Unzoom the entire histogram, x and y.|
| **x**  | Make and display a total projection of the x-axis.|
| **X**  | Make and display a one bin projection of the x-axis, arrow up/down will cycle through all bins.|
| **y**  | Make and display a total projection of the y-axis.|
| **Y**  | Make and display a one bin projection of the y-axis, arrow up/down will cycle through all bins.|


## TGraph/TGraphErrors Commands
|  Key  | Action |:
|:------|:------|
|        | All normal root commands/interactions.|
| **p**  | Print the graph to the terminal.

