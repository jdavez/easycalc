
EasyCalc Pocket PC (PPC) port
========================================================================

Installation & run instructions:
--------------------------------
- Pick the PPC2003, WM5 or WM6 zip file as appropriate for your PDA or Touch Phone.

- From the zip file: copy on a PC, unzip, and copy contents to a directory
  in your PDA / touch phone through the explorer, when your PDA / touch
  phone is in its craddle or connected to the PC.
  You can start the explorer window from ActiveSync (under XP) or
  from the Windows Mobile Device Center (under Vista).
  Or directly in a standard Explorer window, and expand the "Mobile
  Device" subtree.

- If you do not want to copy the contents of the docs subdirectory, you can,
  but then the Help->Help menu item will start Internet Explorer with
  an error as it won't find the manual.html page.

- To run the application, in your PDA or Touch Phone, use the File explorer,
  go to the directory where you installed it, click on EasyCalc(.exe).

- A .cab will come later ...


Uninstall:
----------
- Simply delete the following files from the place where they were put:
	EasyCalc.exe
	EasyCalc*.gif
	EasyCalc*.layout
	lang.rcp
	state.bin

Releases:
---------
* 1.25g-1 PPC:
  - Corrected legacy code behavior when selecting curve for tracking or intersection,
    show only in list graph curves which have been enabled for display.
  - Corrected a black line bug in the result area of the calculator, showing in what
    seems to be rare cases - but at least in the case of one HTC (cf. bug tracker #3256954).

* 1.25g PPC:
  - Graphes at last, but not least ...! (lots of features in there, that
    took me time).
  - Lower memory consumption with skins (this was causing some problems
    for 640 x 480 resolutions).
  - Tried an adaptive mode if not enough memory, where a skin can be removed
    from memory to free space for another one to switch to at switch time
    if there is not enough room to hold them all. Let me know if you are in
    such a situation and still have some trouble. The calculator should be able
    run with only one skin loaded at any time in memory, that just takes a
    little more time when switching from one screen to another (1, 2, 3 and G).
  - Fixed bug when clicking the V button in a variable modification dialog,
    or clicking the F button in a function modification dialog, would corrupt
    the name of the variable / function being modified and result in
    some unpredictable behavior.
  - Added support for the old Pocket PC 2003 platform. An ARM Windows CE 4.02
    package is now also produced and available, besides the WM5 and WM6 packages.
    Enjoy !
  - Corrected a bug in dynamic mouse mode where going over the "0" button was not
    activating it like other buttons.
  - Modified main skins to make annunciators more visible, and to make it
    more intuitive that one can "click" or tap on them to access functions.
  - Can now drag results left or right when total lenght is wider than display area.
  - Corrected a bug for random nunmber generation (seed was not handled properly,
    resulting in non random lists generated ..).

* 1.25f PPC:
  - Removed some memory leaks.
  - Fixed some bugs in the code.
  - Added skin support, and two additional standard skins. Now 3 skins
    can be accessed and switched between at any time by tapping on screen
    annunciators 1, 2 and 3.
  - Added a number of physics constants, accessible from skin 3.
  - Added inline help strings for a number of functions.
  - Added matching (, ), [ and ] highlighting on the input text area,
    whether using keyboard, SIP or buttons.
  - Limited history to 100 entries, as the history popup can be long to appear
    when history is lengthy. If somebody needs more, let me know.
  - Pocket PC specific documentation (in english), which can be accessed
    through the Help menu.

* 1.25e PPC:
  - Fixed bug from legacy code where matrix[n:.] which is resulting in a
    list is not recognized as a list and cannot be edited in list editor
    by a tap on the result area.
  - Modified torad(), todeg(), todeg2() so that they take into account
    current trigonometric mode, and do the proper conversion.
  - Added tograd() function.
  - These toxxx functions also now bring back the value to the proper
    range, i.e. ]-2pi,2pi[ for radian, ]-360,360[ for degree and degree2,
    and ]-400,400[ for grad.
  - Modified the M menu at right of result screen so that it is able to
    call these conversion functions at any time when there is a real or
    complex number in ANS, and not just function of the trigonometric mode.
  - Added mouse position integration code to better filter out spurious
    signals when stylus is leaving screen. This problem is mostly
    happening on the Asus A639 I suppose, but that doesn't hurt, and anyway
    it brings stability when hitting keys on the screen with fingers, in
    case you want to use this app just as you would with a normal calculator.
  - Added Solver and Solver interface.
  - Added Financial Manager.
  - Added Import / Export of variables, functions and equations to text
    files, for exchange with other people or platforms, and backup.
  - Fixed a number of bugs in the code.
  - Adapt better to larger resolutions than 240 x 320 (e.g. VGA 480 x 640).
  - Added feature request 2875982, extended it (be able to convert displayed
    result to normal, engineer or scientific mode as needed).

* 1.25d PPC:
  - More stringent window handling code to fight further the few cases
    remaining where the application "disappear" when coming back from
    power suspend mode. Looks more stable now.
  - Modified the DataManager window to show column headings and make
    them resizable by the user.
  - Added "=" (press on shift + =) key to allow variable assignment directly
    from normal pad and without using the SIP/keyboard.
  - Added the matrix editor.
  - Added old documentation of special functions in the release package.

* 1.25c PPC:
  - Error in .layout file, EXP/EE key was pointing at ')'.
  - Bug in text selection (coming from legacy code) after key text insert,
    which was causing x^y and x^(1/y) to select wrong length of text
    when at beginning of input line.
  - Added the ° (degree) and ' (minutes) keys as shift + '*' and '-'.

* 1.25b PPC:
  - Display bugs on togonio() and torad() fixed.
  - Display of text with power exponents can now be scrolled.
  - Improved lifecycle of some graphical objects to fight a tendancy of
    the application to "disappear" when coming back from power suspend
    mode ... from time to time.
  - Simplified some of the code imported from Free42 interface.
  - Implemented ask() function.
  - Full list support, with list editor added.
  - Set selection of text and default focus to edit boxes in dialogs
    for ease of use.

* 1.25a PPC:
  - initial PPC port release


Functional differences between 1.25g PPC and legacy Palm EasyCalc 1.25:
-----------------------------------------------------------------------
- Fully skinable: one can create new skins (.gif and .layout files) to
  customize EasyCalc look as one wants, including redesigning buttons
  and their function or placement, "à-la" free42.
  This doesn't require recompilation nor special skills, except some
  image crafting plus understanding the .layout syntax.

- Dynamic change of skins: change them on the fly through the options
  panel.

- The old B / S / I approach has been abandonned as being too specific.
  Any skin including the standard supplied one can have up to 3 customizable
  panels + graph, which can be accessed at any time by a tap on corresponding
  annunciators at top of screen.

- No special menu either, this is managed through skins (we could add more
  active skins to switch between if needed).

- Dynamic switch of language on the UI, by just going to the options panel.
  No more several versions to pick from.
  When starting for the first time, EasyCalc PPC determines the user
  language from the system, and adopts it if it knows it, else defaults
  to english.

- Adding or modifying a language doesn't require re-compilation, just a
  Windows machine with an editor, and a click on a batch file to generate
  the merged lang.rcp language file. Then transfer lang.rcp to the directory
  where EasyCalc PPC is installed on your PDA, and next time it starts, the
  new or modified language will be available and taken into account.
  Notes: a) If you modify a language file or add a new one, please post it
            in the Feature Requests Tracker of EasyCalc on SourceForge
            http://sourceforge.net/tracker/?group_id=13138&atid=363138
            so that can be accounted for in the next release of EasyCalc PPC.
         b) For now, I didn't include in the package the merge.bat file
            nor all the separate language files. Signal your will to
            translate in the forum or in the feature requests tracker,
            and we will convene on a way to get your input in the release
            package.

- Not yet translated objects in a chosen language appear on the UI
  as $$XXX, where XXX is the unique token to translate, originally in
  english. To resolve that, just add a "$$XXX" = "<translated_text>"
  in the corresponding language .rcp file, execute merge.bat, and
  push the new lang.rcp on the Pocket PC as described above.
  And don't forget to post your work to the Tracker ;-)

- An imprecision on sqrt() with negative or complex numbers is resolved:
  sqrt(-1) was returning a complex value which had a non 0 real part, whose
  value was depending on the precision of float / double / math means used
  behind calculations. I.e., sqrt(-1) was returning 6.12303176911E-17 + 1i
  on my PPC, and  0.000000000000147 + 1i on my Sony Clié emulation.
  This is now corrected, and it returns for example +1i in this case.

- An imprecision on y^x with complex numbers or negative y with non integral x
  is resolved:
  i^2 was returning something like -1+1,0106431E-15i (again subject to the
  precision of calculations behind).
  Now this example calculation returns a proper -1.

- An infinite loop has been corrected, when displaying 0 in Engineer mode.

- Added the capability to enter and manipulate infinity (= Inf) in formulae
  and in the input area.

- Minor text insert difference on ^(), 2^() and 10^(), where if text is
  selected, it is put between brackets instead of being overwritten.

- Corrected bug in text selection after key text insert, which was causing
  x^y and x^(1/y) to select wrong length of text when at beginning of input
  line. This was only showing on x^(1/y) in the legacy version, because of
  the text insert difference on x^y between the two versions.

- ask() panel has the classical V, F and f buttons to enter text (no button
  in the legacy Palm app, just an input text area).

- The history popup does not align differently (left and right) text coming
  from results or input areas. Windows doesn't allow that in standard list
  boxes, items must be aligned all the same.

- In List Editor, added an Append button in addition to Insert, to add item(s)
  at end of list.

- In List Editor, New list, Append and Insert now loop on getting values,
  in order to ease user task when entering new lists, or when growing them.
  Press Cancel to end the loop.

- matrix[n:.] which is resulting in a list is recognized as a list by the PPC
  version and can be edited in list editor by a tap on the result area.

- Solver interface has the classical V, F and f buttons to enter text (no button
  in the legacy Palm app, just an input text area).

- torad(), todeg(), todeg2() take into account the current trigonometric mode,
  and do proper conversion. Added tograd() function.These functions also now
  bring back the value to the proper range, i.e. ]-2pi,2pi[ for radian,
  ]-360,360[ for degree and degree2, and ]-400,400[ for grad.

- The M menu at right of result is able to call these conversion functions at
  any time when there is a real or complex number in ANS, and not just function
  of the trigonometric mode.

- Financial Manager:
    - show label of selected variable.
    - Require double tap, or tap again, on variable value to edit it, instead
      of single tap.
    - Show total payment and total cost (= total payment + remaining - initial value).
    - Fixed bugs on the sign of formulae or on Present and Future values in
      some calculations.
    - Improved precision on some calculations.
    - No change on PYR value when clicking on it, if I = 0.

- Integrated solution from lizzybarham to feature request 2875982, and extended
  it to be able to convert displayed result to normal, engineer or scientific mode
  as needed (feature request was only for scientific).

- Import / export of equations done from main menu, and not from Solver menu.

- Export is either on variables, functions or equations, but not on
  variables + functions.

- Sped up the display_matrix() and cvt_fltoa() routines for big matrices (50 x 50).

- Some physical constants have been integrated, using more recent values than in
  the PalmOS manual, mostly coming from http://physics.nist.gov/cuu/Constants/index.html.
   euler gamma=0.57721 56649 01532 86060 65120 90082 40243 10421 59335 93992 ...
   Speed of light in vacuum (m s-1) c=299792458
   Newtonian constant of gravitation (m3 kg-1 s-2) G=6.67428E-11
   Standard gravitational acc. (N kg-1) g=9.80665
   Electron mass (kg) me=9.10938215E-31
   Proton mass (kg) mp=1.672621637E-27
   Neutron mass (kg) mn=1.67492729E-27
   Unified Atomic mass unit (kg) u=1.660538782E-27
   Planck constant (J s) h=6.62606896-34
   Boltzmann constant (J K-1) k=1.380 6504-23
   Magnetic permeability - vacuum (magnetic constant) (H m-1) µ0=1.2566370614E-6
   Dielectric permittivity (electric constant) (F m-1) e0=8.854187817E-12
   Fine structure constant alpha=7.2973525376E-3
   Rydberg constant (m-1) r=10973731.568527
   Classical electron radius (m) re=2.8179402894E-15
   Bohr radius (m) a0=5.2917720859E-11
   Fluxoid quantum (magnetic flux quantum) (W b) phi0=2.067833667E-15
   Bohr magneton (J T-1) µb=9.27400915E-24
   Electron magnetic moment (J T-1) µe=-9.28476377E-24
   Nuclear magneton (J T-1) µN=5.05078324E-27
   Proton magnetic moment (J T-1) µp=1.410606662E-26
   Neutron magnetic moment (J T-1) µn=-9.6623641E-27
   Compton wavelength (electron) (m) lc=2.4263102175E-12
   Compton wavelength (proton) (m) lcp=1.3214098446E-15
   Stefan-Boltzmann constant (W m-2 K-4) sigma=5.670400E-8
   Avogadro's constant (mol-1) Na=6.02214179E+23
   Ideal gas volume at STP (m3 mol-1) Vm=2.2413996E-2
   Universal molar gas constant (J mol-1 K-1) R=8.314472
   Electron charge (elementary charge) (C) e=1.602176487E-19
   Faraday constant (C mol-1) F=96485.3399
   Quantum Hall resistance = von Klitzing constant (Ohm) Rk=25812.807

- Completed inline help strings for a number of functions.

- Limited history to 100 entries, as the history popup can be long to appear
  when history is lengthy. If somboedy needs mode, let me know (this is
  unlimited in legacy code).

- In the graph setup panel, bug fixed: when entering a value for say Y2:,
  which creates z_grafun2(), then using z_grafun2() for Y1: by pressing
  on the Y1: label, and then pressing the Y2: label to get the list of
  functions ans selecting "Delete", the contents of Y1: would stay for
  ever on the non defined function z_grafun2(), with no way out: pressing Delete
  on this one does not empty the box / selection, and clicking on the edit text
  can only modify the z_grafun2() function, and not z_grafun1().

- When entering in Graph mode, reduce precision was set to false, restoring the
  initial value later. However, this was showing in Preferences, and modifiable
  by user, which could modify graphing behavior. This is now invisible to the user.

- The graph skin philosophy and usability is largely different from initial
  code, which I felt was not intuitive. I hope this one is better to use.
  Let me know.

- There is now a track view display to see a larger zone and to move the graph zone
  on it in order to explore in details wider parts more easily.

- The selection mode on curve, allowing to pick a calculated point on curve
  by tapping on the screen, is different from Palm code in Polar mode: the stylus y
  position is ignored, and only the x position is used to select a value between
  th-min and th-max, in the same way as the parametric mode curve point selection.
  Initial code was calculating the intersection of (O, tap_point) and curve, and
  selecting that point, which was non obvious to use when O was not visible, and
  did not allow to select some points on drawn curve when it was cycling several
  times.

- Each mode (function, polar, parametric) has its own set of activated curves,
  while this was a single common set in the initial code for all modes. This made
  little sense.

- There is no "Table mode" dialog for graph functions. I didn't see it was worth
  the trouble creating this view. If you really need it, let me know.

- Added a 'square' button to restore the graph zone to a normed view where 1 vertical
  = 1 horizontal.

- Corrected legacy code behavior when selecting curve for tracking or intersection,
  show only in list graph curves which have been enabled for display.


Still missing in 1.25g:
-----------------------
- Support screen rotation with another set of skins (EasyCalcWide.layout + gif).
- Updated help.
- ... that should be all ...


Bugs:
-----
- Report bugs at:
  http://sourceforge.net/projects/easycalc/forums/forum/41375
  or
  http://sourceforge.net/tracker/?group_id=13138&atid=113138

Mapi.