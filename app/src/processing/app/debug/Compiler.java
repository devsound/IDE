/* -*- mode: java; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
  Part of the Processing project - http://processing.org

  Copyright (c) 2004-08 Ben Fry and Casey Reas
  Copyright (c) 2001-04 Massachusetts Institute of Technology

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

package processing.app.debug;

import processing.app.Base;
import processing.app.Preferences;
import processing.app.Sketch;
import processing.app.SketchCode;
import processing.core.*;
import processing.app.I18n;
import static processing.app.I18n._;

import java.io.*;
import java.util.*;
import java.util.zip.*;

//import java.io.PrintWriter.*;

public class Compiler implements MessageConsumer {
  static final String BUGS_URL =
    _("http://github.com/devsound/IDE/issues");
  static final String SUPER_BADNESS =
    I18n.format(_("Compiler error, please submit this code to {0}"), BUGS_URL);

  Sketch sketch;
  String buildPath;
  String primaryClassName;
  boolean verbose;
  boolean sketchIsCompiled;
  int objectCount;
  int objectsCompiled;

  RunnerException exception;
  
  class ObjectDeps {
    public String module;
    public ArrayList imports;
    public ArrayList exports;

    public ObjectDeps(String m, ArrayList nm) {
      module = m;
      imports = new ArrayList();
      exports = new ArrayList();
      for(int n = 0; n < nm.size(); n++) {
        String[] sym = ((String)nm.get(n)).split(" ");
        if(sym[1].equals("U")) {
          imports.add(sym[0]);
        } else {
          exports.add(sym[0]);
        }
      }
    }
  }

  public Compiler() { }

  /**
   * Compile with avr-gcc.
   *
   * @param sketch Sketch object to be compiled.
   * @param buildPath Where the temporary files live and will be built from.
   * @param primaryClassName the name of the combined sketch file w/ extension
   * @return true if successful.
   * @throws RunnerException Only if there's a problem. Only then.
   */
  private boolean compile_avr(Sketch sketch,
                         String buildPath,
                         String primaryClassName,
                         boolean verbose) throws RunnerException {
    this.sketch = sketch;
    this.buildPath = buildPath;
    this.primaryClassName = primaryClassName;
    this.verbose = verbose;
    this.sketchIsCompiled = false;

    // the pms object isn't used for anything but storage
    MessageStream pms = new MessageStream(this);

    String avrBasePath = Base.getAvrBasePath();
    Map<String, String> boardPreferences = Base.getBoardPreferences();
    String core = boardPreferences.get("build.core");
    if (core == null) {
    	RunnerException re = new RunnerException(_("No board selected; please choose a board from the Tools > Board menu."));
      re.hideStackTrace();
      throw re;
    }
    String corePath;
    
    if (core.indexOf(':') == -1) {
      Target t = Base.getTarget();
      File coreFolder = new File(new File(t.getFolder(), "cores"), core);
      corePath = coreFolder.getAbsolutePath();
    } else {
      Target t = Base.targetsTable.get(core.substring(0, core.indexOf(':')));
      File coreFolder = new File(t.getFolder(), "cores");
      coreFolder = new File(coreFolder, core.substring(core.indexOf(':') + 1));
      corePath = coreFolder.getAbsolutePath();
    }

    String variant = boardPreferences.get("build.variant");
    String variantPath = null;
    
    if (variant != null) {
      if (variant.indexOf(':') == -1) {
	Target t = Base.getTarget();
	File variantFolder = new File(new File(t.getFolder(), "variants"), variant);
	variantPath = variantFolder.getAbsolutePath();
      } else {
	Target t = Base.targetsTable.get(variant.substring(0, variant.indexOf(':')));
	File variantFolder = new File(t.getFolder(), "variants");
	variantFolder = new File(variantFolder, variant.substring(variant.indexOf(':') + 1));
	variantPath = variantFolder.getAbsolutePath();
      }
    }

    List<File> objectFiles = new ArrayList<File>();

   // 0. include paths for core + all libraries

   sketch.setCompilingProgress(20);
   List includePaths = new ArrayList();
   includePaths.add(corePath);
   if (variantPath != null) includePaths.add(variantPath);
   for (File file : sketch.getImportedLibraries()) {
     includePaths.add(file.getPath());
   }

   // 1. compile the sketch (already in the buildPath)

   sketch.setCompilingProgress(30);
   objectFiles.addAll(
     compileFiles(avrBasePath, buildPath, includePaths,
               findFilesInPath(buildPath, "S", false),
               findFilesInPath(buildPath, "c", false),
               findFilesInPath(buildPath, "cpp", false),
               boardPreferences));
   sketchIsCompiled = true;

   // 2. compile the libraries, outputting .o files to: <buildPath>/<library>/

   sketch.setCompilingProgress(40);
   for (File libraryFolder : sketch.getImportedLibraries()) {
     File outputFolder = new File(buildPath, libraryFolder.getName());
     File utilityFolder = new File(libraryFolder, "utility");
     createFolder(outputFolder);
     // this library can use includes in its utility/ folder
     includePaths.add(utilityFolder.getAbsolutePath());
     objectFiles.addAll(
       compileFiles(avrBasePath, outputFolder.getAbsolutePath(), includePaths,
               findFilesInFolder(libraryFolder, "S", false),
               findFilesInFolder(libraryFolder, "c", false),
               findFilesInFolder(libraryFolder, "cpp", false),
               boardPreferences));
     outputFolder = new File(outputFolder, "utility");
     createFolder(outputFolder);
     objectFiles.addAll(
       compileFiles(avrBasePath, outputFolder.getAbsolutePath(), includePaths,
               findFilesInFolder(utilityFolder, "S", false),
               findFilesInFolder(utilityFolder, "c", false),
               findFilesInFolder(utilityFolder, "cpp", false),
               boardPreferences));
     // other libraries should not see this library's utility/ folder
     includePaths.remove(includePaths.size() - 1);
   }

   // 3. compile the core, outputting .o files to <buildPath> and then
   // collecting them into the core.a library file.

   sketch.setCompilingProgress(50);
  includePaths.clear();
  includePaths.add(corePath);  // include path for core only
  if (variantPath != null) includePaths.add(variantPath);
  List<File> coreObjectFiles =
    compileFiles(avrBasePath, buildPath, includePaths,
              findFilesInPath(corePath, "S", true),
              findFilesInPath(corePath, "c", true),
              findFilesInPath(corePath, "cpp", true),
              boardPreferences);

   String runtimeLibraryName = buildPath + File.separator + "core.a";
   List baseCommandAR = new ArrayList(Arrays.asList(new String[] {
     avrBasePath + "avr-ar",
     "rcs",
     runtimeLibraryName
   }));
   for(File file : coreObjectFiles) {
     List commandAR = new ArrayList(baseCommandAR);
     commandAR.add(file.getAbsolutePath());
     execAsynchronously(commandAR, null);
   }

    // 4. link it all together into the .elf file
    // For atmega2560, need --relax linker option to link larger
    // programs correctly.
    String optRelax = "";
    String atmega2560 = new String ("atmega2560");
    if ( atmega2560.equals(boardPreferences.get("build.mcu")) ) {
        optRelax = new String(",--relax");
    }
   sketch.setCompilingProgress(60);
    List baseCommandLinker = new ArrayList(Arrays.asList(new String[] {
      avrBasePath + "avr-gcc",
      "-Os",
      "-Wl,--gc-sections"+optRelax,
      "-mmcu=" + boardPreferences.get("build.mcu"),
      "-o",
      buildPath + File.separator + primaryClassName + ".elf"
    }));

    for (File file : objectFiles) {
      baseCommandLinker.add(file.getAbsolutePath());
    }

    baseCommandLinker.add(runtimeLibraryName);
    baseCommandLinker.add("-L" + buildPath);
    baseCommandLinker.add("-lm");

    execAsynchronously(baseCommandLinker, null);

    List baseCommandObjcopy = new ArrayList(Arrays.asList(new String[] {
      avrBasePath + "avr-objcopy",
      "-O",
      "-R",
    }));

    List commandObjcopy;

    // 5. extract EEPROM data (from EEMEM directive) to .eep file.
    sketch.setCompilingProgress(70);
    commandObjcopy = new ArrayList(baseCommandObjcopy);
    commandObjcopy.add(2, "ihex");
    commandObjcopy.set(3, "-j");
    commandObjcopy.add(".eeprom");
    commandObjcopy.add("--set-section-gccFlags=.eeprom=alloc,load");
    commandObjcopy.add("--no-change-warnings");
    commandObjcopy.add("--change-section-lma");
    commandObjcopy.add(".eeprom=0");
    commandObjcopy.add(buildPath + File.separator + primaryClassName + ".elf");
    commandObjcopy.add(buildPath + File.separator + primaryClassName + ".eep");
    execAsynchronously(commandObjcopy, null);
    
    // 6. build the .hex file
    sketch.setCompilingProgress(80);
    commandObjcopy = new ArrayList(baseCommandObjcopy);
    commandObjcopy.add(2, "ihex");
    commandObjcopy.add(".eeprom"); // remove eeprom data
    commandObjcopy.add(buildPath + File.separator + primaryClassName + ".elf");
    commandObjcopy.add(buildPath + File.separator + primaryClassName + ".hex");
    execAsynchronously(commandObjcopy, null);
    
    sketch.setCompilingProgress(90);
   
    return true;
  }
  
  String fixMacPath(String rel) {
    if (Base.isMacOS()) {
      String macospath = getClass().getProtectionDomain().getCodeSource().getLocation().getPath();
      return new File(macospath).getParent() + "/" + rel;
    } else {
      return rel;
    }
  }
  
  String getCorePath(String core) {
    if (core.indexOf(':') == -1) {
      Target t = Base.getTarget();
      File coreFolder = new File(new File(t.getFolder(), "cores"), core);
      return coreFolder.getAbsolutePath();
    } else {
      Target t = Base.targetsTable.get(core.substring(0, core.indexOf(':')));
      File coreFolder = new File(t.getFolder(), "cores");
      coreFolder = new File(coreFolder, core.substring(core.indexOf(':') + 1));
      return coreFolder.getAbsolutePath();
    }
  }

  String makeRelative(String absolute, String base) {
    if(absolute.startsWith(base)) {
      if(absolute.length() == base.length()) return "";
      return absolute.substring(base.length() + 1);
    }
    return absolute;
  }

  ArrayList concat(ArrayList a, ArrayList b) {
    ArrayList out = new ArrayList();
    if(a != null) for(int i = 0; i < a.size(); i++) out.add(a.get(i));
    if(b != null) for(int i = 0; i < b.size(); i++) out.add(b.get(i));
    return out;
  }

  ArrayList recursiveCompile(String tools, String gnucc, ArrayList cFlags, ArrayList cxxFlags, ArrayList gccFlags, File root, File pathspec, boolean noCompile, boolean noCheck) throws RunnerException {
    ArrayList objectFiles = new ArrayList();
    if(pathspec == null) pathspec = root;
    if(pathspec.isDirectory()) {
      // Recurse directories
      String files[] = pathspec.list();
      for(String file : files) {
        File path = new File(pathspec, file);
        ArrayList newFiles = recursiveCompile(tools, gnucc, cFlags, cxxFlags, gccFlags, root, path, noCompile, noCheck);
        // Collect object files
        for(int i = 0; i < newFiles.size(); i++) {
          objectFiles.add((String)newFiles.get(i));
        }
      }
    } else {
      // Check extension
      String filename = pathspec.getName();
      if(filename.endsWith(".c") || filename.endsWith(".cpp")) {
        // Strip (but store) extrension
        String ext = filename.substring(filename.lastIndexOf("."));
        filename = filename.substring(0, filename.lastIndexOf("."));
        // Filenames
        String inputFile = pathspec.getPath();
        String outputFile = pathspec.getPath() + ".o";
        Boolean changed = noCheck;
        File src = new File(inputFile);
        File dst = new File(outputFile);
        if(!changed) {
          if(!dst.exists()) {
            changed = true;
          } else {
            changed = (src.lastModified() != dst.lastModified());
          }
        }
        // Pass the output file
        if(changed) {
          objectFiles.add(makeRelative(outputFile, root.getPath()));
          if(!noCompile) {
            // Construct command to execute GCC
            ArrayList compile = new ArrayList();
            compile.add(tools + gnucc);
            if(ext.equals(".c")) {
              for(int i = 0; i < cFlags.size(); i++) {
                compile.add(cFlags.get(i));
              }
            }
            if(ext.equals(".cpp")) {
              compile.add("-xc++");
              for(int i = 0; i < cxxFlags.size(); i++) {
                compile.add(cxxFlags.get(i));
              }
            }
            for(int i = 0; i < gccFlags.size(); i++) {
              compile.add((String)gccFlags.get(i));
            }
            // Add input/output files
            compile.add("-c");
            compile.add("-o" + makeRelative(outputFile, root.getPath())); // Output
            compile.add(makeRelative(inputFile, root.getPath()));         // Input
            // Execute GCC
            execAsynchronously(compile, root);
            dst.setLastModified(src.lastModified());
            objectsCompiled++;
            // Update progress
            sketch.setCompilingProgress(10 + (objectsCompiled * 80) / objectCount);
          }
        }
      }
    }
    return objectFiles;
  }

  ArrayList gatherDeps(ArrayList objects, String tools, File buildRoot) throws RunnerException {
    ArrayList deps = new ArrayList();
    for(int n = 0; n < objects.size(); n++) {
      String module = (String)objects.get(n);
      ArrayList depList = new ArrayList(
        new ArrayList(Arrays.asList(new String[] {
            tools + "arm-none-eabi-nm", "-P", "-g", module
        }))
      );
      // Execute NM to list symbols
      ArrayList nm = execExternTool(depList, buildRoot);
      // Process NM output into dependency list
      deps.add(new ObjectDeps(module, nm));
    }
    return deps;
  }

  ObjectDeps findObject(ArrayList deps, String object) {
    for(int n = 0; n < deps.size(); n++) {
      ObjectDeps o = (ObjectDeps)deps.get(n);
      if(o.module.equals(object)) return o;
    }
    return null;
  }

  ObjectDeps findExportingObject(ArrayList deps, String symbol) {
    for(int n = 0; n < deps.size(); n++) {
      ObjectDeps o = (ObjectDeps)deps.get(n);
      for(int z = 0; z < o.exports.size(); z++) {
        if(((String)o.exports.get(z)).equals(symbol)) return o;
      }
    }
    return null;
  }
  
  String devFromTarget(String t) {
    if(t == null) t = "";
    return t.split("_")[0];
  }
  
  String revFromTarget(String t) {
    if(t == null) t = "";
    if(t.indexOf("_") == -1) t += "_R001";
    String r = t.split("_")[1];
    if(r.charAt(0) == 'R') r = r.substring(1);
    while(r.charAt(0) == '0') r = r.substring(1);
    return r;
  }
  
  ArrayList gatherObjects(String origin, ArrayList deps) throws RunnerException {
    ArrayList depList = new ArrayList();
    depList.add(findObject(deps, origin));
    if(verbose) System.out.println("Dependency tree:");
    for(int n = 0; n < depList.size(); n++) {
      ObjectDeps o = (ObjectDeps)depList.get(n);
      if(verbose) System.out.print(o.module);
      for(int z = 0; z < o.imports.size(); z++) {
        ObjectDeps e = findExportingObject(deps, (String)o.imports.get(z));
        if(e != null) {
          if(findObject(depList, e.module) == null) {
            depList.add(e);
          }
          if(verbose) System.out.print(" <= " + e.module);
        }
      }
      if(verbose) System.out.println("");
    }
    ArrayList objects = new ArrayList();
    for(int n = 0; n < depList.size(); n++) {
      objects.add(((ObjectDeps)depList.get(n)).module);
    }
    return objects;
  }

  public boolean compile_arm(Sketch sketch, String buildPath, String primaryClassName, boolean verbose) throws RunnerException {

    // Preferences
    Map<String, String> prefs = Base.getBoardPreferences();

  	// Path to GNU tools for ARM embedded (arm-none-eabi binaries)
  	String tools     = Base.getArmBasePath();
  	
  	// Path to environment (processor specific code)
    String env       = fixMacPath("hardware/tools/" + prefs.get("build.tools"));
  	
  	// Path to core (platform specific code)
    String core      = fixMacPath("hardware/tools/" + prefs.get("build.core"));

    // Verbosity override
    if(Preferences.getBoolean("build.verbose")) verbose = true;

    // Prepare
    this.sketch           = sketch;
    this.buildPath        = buildPath;
    this.primaryClassName = primaryClassName;
    this.verbose          = verbose;
    this.sketchIsCompiled = false;

    // List of include paths
    List includePaths = new ArrayList();
    includePaths.add("include");               // Default environment
    includePaths.add("lib");                   // Default environment library
    includePaths.add(prefs.get("build.core")); // Default platform

    // Copy environment
    System.out.println("Preparing build environment...");
    File srcPath = new File(env);
    File dstPath = new File(buildPath);
    //make sure source exists
    if(!srcPath.exists()) {
      System.out.println("Your tools are not tooling!");
    } else {
      try {
        copyFolder(srcPath, dstPath);
      } catch(IOException e){
        e.printStackTrace();
      }
    }
    srcPath = new File(core);
    dstPath = new File(buildPath + File.separator + prefs.get("build.core"));
    //make sure source exists
    if(!srcPath.exists()) {
      System.out.println("Your core is not coring!");
    } else {
      try {
        copyFolder(srcPath, dstPath);
      } catch(IOException e){
        e.printStackTrace();
      }
    }

    sketch.setCompilingProgress(5);

    // Copy libraries, remember to include
    for(File file : sketch.getImportedLibraries()) {
      includePaths.add(file.getName());
      try {
        copyFolder(file, new File(buildPath + File.separator + file.getName()));
      } catch(IOException e){
        e.printStackTrace();
      }
    }

    sketch.setCompilingProgress(10);

    // It's compilation time
    System.out.println("Compiling...");
    String gnucc = "arm-none-eabi-gcc";

    // Flags for GCC
    ArrayList flags = new ArrayList(Arrays.asList(new String[] {
      "-ggdb3", "-Os", "-Wall", "-Wno-unused-function", "-Wno-main", "-mcpu=cortex-m4", "-msoft-float",
      "-mthumb", "-ffunction-sections", "-fdata-sections", "-fno-builtin",
      "-fstrict-volatile-bitfields", "-flto", "-fno-use-linker-plugin"//, "-mlong-calls" //, "-g"
    }));
    // Flags specific to C
    ArrayList cFlags = new ArrayList(Arrays.asList(new String[] {
      "-fplan9-extensions", "-std=gnu11"
    }));
    // Flags specific to C++
    ArrayList cxxFlags = new ArrayList(Arrays.asList(new String[] {
      "-fno-rtti", "-fno-exceptions"
    }));

    ArrayList gccFlags = (ArrayList)flags.clone();

    // Defines that guide compilation
    gccFlags.add("-DF_CPU=48000000");                                       // CPU frequency
    try {
      gccFlags.add("-DEXTERNAL_XTAL=" + prefs.get("build.xtal").toString());  // Crystal frequency
    } catch(Exception e) {}
    gccFlags.add("-DDEVICE=" + devFromTarget(prefs.get("build.target")));   // Device
    gccFlags.add("-DUDAD_REV=" + revFromTarget(prefs.get("build.target"))); // Revision

    // Include paths
    for(int i = 0; i < includePaths.size(); i++) {
      gccFlags.add("-I" + (String)includePaths.get(i));
    }

    String binTools = tools + "bin" + File.separator;
    File buildRoot = new File(buildPath);

    // Count files to compile (do not compile, check timestamps)
    ArrayList objects = recursiveCompile(binTools, gnucc, cFlags, cxxFlags, gccFlags, buildRoot, null, true, false);
    objectCount = objects.size();
    objectsCompiled = 0;

    // Perform compilation  (compile, check timestamps)
    recursiveCompile(binTools, gnucc, cFlags, cxxFlags, gccFlags, buildRoot, null, false, false);
    
    // Return all object files (do not compile, ignore timestamps)
    objects = recursiveCompile(binTools, gnucc, cFlags, cxxFlags, gccFlags, buildRoot, null, true, true);

    // Gather dependencies
    System.out.println("Updating dependencies...");
    ArrayList deps = gatherDeps(objects, binTools, buildRoot);
    
    // Gather objects using the dependency tree
    objects = gatherObjects("__startup.c.o", deps);

    // It's linking time
    System.out.println("Linking...");
    ArrayList link;

    sketch.setCompilingProgress(92);

    // Link files
    link = new ArrayList(
      concat(
        concat(
          concat(
            new ArrayList(Arrays.asList(new String[] {
                binTools + "arm-none-eabi-g++", "-osketch.elf"
            })),
            concat(flags, cxxFlags)
          ),
          new ArrayList(Arrays.asList(new String[] {
            "-Wl,--gc-sections", "-fwhole-program", "-Tld/MK20DX32VFM5.ld", "-nostartfiles", "-Wl,-Map=sketch.map"
          }))
        ),
        objects
      )
    );
    execAsynchronously(link, buildRoot);

    sketch.setCompilingProgress(98);

    // Convert ELF to BIN
    link = new ArrayList(
      new ArrayList(Arrays.asList(new String[] {
        binTools + "arm-none-eabi-objcopy", "-O", "binary", "sketch.elf", "sketch.bin"
      }))
    );
    execAsynchronously(link, buildRoot);

    sketch.setCompilingProgress(100);

    System.out.println("");
    
    return true;
  }
  /**
   * Compile with whatever makefile.
   *
   * @param sketch Sketch object to be compiled.
   * @param buildPath Where the temporary files live and will be built from.
   * @param primaryClassName the name of the combined sketch file w/ extension
   * @return true if successful.
   * @throws RunnerException Only if there's a problem. Only then.
   */
  public boolean compile(Sketch sketch,
                         String buildPath,
                         String primaryClassName,
                         boolean verbose) throws RunnerException {

    Map<String, String> boardPreferences = Base.getBoardPreferences();

    String core = boardPreferences.get("build.core");
    if(core == null) {
    	RunnerException re = new RunnerException(_("No board selected; please choose a board from the Tools > Board menu."));
      re.hideStackTrace();
      throw re;
    }

    System.out.println("Now building for " + core + "...");
    System.out.println("");
    if (verbose || Preferences.getBoolean("build.verbose")) System.out.println(buildPath);
    
    if(core.equals("arduino")) {
      return compile_avr(sketch, buildPath, primaryClassName, verbose);
    }

    if(1 == 1) {
      return compile_arm(sketch, buildPath, primaryClassName, verbose);
    }
                          
  	String armBasePath = Base.getArmBasePath();

   
    this.sketch = sketch;
    this.buildPath = buildPath;
    this.primaryClassName = primaryClassName;
    this.verbose = verbose;
    this.sketchIsCompiled = false;

   // 0. include paths for core + all libraries

    // the pms object isn't used for anything but storage
    MessageStream pms = new MessageStream(this);

    String armToolchain;
    String tools = boardPreferences.get("build.tools");

    if (Base.isMacOS()) {
      String macospath = getClass().getProtectionDomain().getCodeSource().getLocation().getPath();
      armToolchain = new File(macospath).getParent() + "/hardware/tools/devsound/";
      if(tools != null) tools = new File(macospath).getParent() + "/hardware/tools/" + tools + "/";
    } else {
      armToolchain = "hardware/tools/devsound/";
      if(tools != null) tools = "hardware/tools/" + tools;
    }
    //System.out.println(armToolchain);
    String corePath;
    
    if (core.indexOf(':') == -1) {
      Target t = Base.getTarget();
      File coreFolder = new File(new File(t.getFolder(), "cores"), core);
      corePath = coreFolder.getAbsolutePath();
    } else {
      Target t = Base.targetsTable.get(core.substring(0, core.indexOf(':')));
      File coreFolder = new File(t.getFolder(), "cores");
      coreFolder = new File(coreFolder, core.substring(core.indexOf(':') + 1));
      corePath = coreFolder.getAbsolutePath();
    }

    String variant = boardPreferences.get("build.variant");
    String variantPath = null;
    
    if(variant != null) {
      if (variant.indexOf(':') == -1) {
        Target t = Base.getTarget();
        File variantFolder = new File(new File(t.getFolder(), "variants"), variant);
        variantPath = variantFolder.getAbsolutePath();
      } else {
        Target t = Base.targetsTable.get(variant.substring(0, variant.indexOf(':')));
        File variantFolder = new File(t.getFolder(), "variants");
        variantFolder = new File(variantFolder, variant.substring(variant.indexOf(':') + 1));
        variantPath = variantFolder.getAbsolutePath();
      }
    }

    List<File> objectFiles = new ArrayList<File>();

    // 0. include paths for core + all libraries
    sketch.setCompilingProgress(20);
    List includePaths = new ArrayList();
    for (File file : sketch.getImportedLibraries()) {
      includePaths.add(file.getPath());
    }
    

    // 0.5 copy the necessary folders from the toolchain to the temp folder
    File srcFolder = new File(armToolchain);
    File destFolder = new File(buildPath);
    if (verbose || Preferences.getBoolean("build.verbose")) System.out.println("Copying tools...");
    //make sure source exists
    if(!srcFolder.exists()){
      System.out.println("I think your devsound is not devsounding");
       //just exit
       //System.exit(0);
    } else {
      try{
        copyFolder(srcFolder,destFolder);
      }catch(IOException e){
        e.printStackTrace();
        //error, just exit
        //System.exit(0);
      }
    }
    
    //System.out.println(tools);
    if(tools != null) {
      srcFolder = new File(tools);
      destFolder = new File(buildPath, "toolchain");
      //make sure source exists
      if(!srcFolder.exists()){
        System.out.println("I think your tools are not tooling");
         //just exit
         //System.exit(0);
      } else {
        try{
          copyFolder(srcFolder,destFolder);
        }catch(IOException e){
          e.printStackTrace();
          //error, just exit
          //System.exit(0);
        }
      }
    }

    String nocppClassName = primaryClassName.substring(0, primaryClassName.lastIndexOf('.')) + ".c";
    // 0.75 copy the sketch into the compilation folder prepared to run with the makefile
    if (verbose || Preferences.getBoolean("build.verbose")) System.out.println("Copying sketch...");
    try {
      copyFile(buildPath + File.separator + primaryClassName, buildPath + File.separator + nocppClassName);
      primaryClassName = nocppClassName;
    } catch(IOException e) {
      e.printStackTrace();
    	//error, just exit
      //System.exit(0);
    }

    if (verbose || Preferences.getBoolean("build.verbose")) System.out.println("Generating makefile...");
    try {
      PrintWriter writer = new PrintWriter(buildPath + File.separator + "Makefile", "UTF-8");
      writer.println("TOOLS='" + armBasePath + "'");
      writer.println("PROG=sketch");
      writer.println("PLATFORM=" + boardPreferences.get("build.core").toString().toUpperCase());
      if(!(boardPreferences.get("build.xtal") == null)) {
        writer.println("XTAL=" + boardPreferences.get("build.xtal").toString());
      }
      if(!(boardPreferences.get("build.target") == null)) {
        writer.println("DEVICE=" + boardPreferences.get("build.target").toString());
      }
      
      if(includePaths.size() > 0) {
        writer.print("CPPFLAGS=");
        for (int i = 0; i < includePaths.size(); i++) {
          writer.print("-I" + (String) includePaths.get(i)+" ");
        }
        writer.println("");
      }
      writer.print("SRCS=");

      ArrayList<File> srcs = findFilesInPath(buildPath, "c", false);
      for(File f : srcs) {
        writer.print(" " + f.getName());
      }
      
      for (int i = 0; i < includePaths.size(); i++) {
          File dir = new File(includePaths.get(i).toString());
          if (verbose || Preferences.getBoolean("build.verbose")) System.out.println("Importing " + dir.getName() + " (" + dir.getPath() + ")");
          for (File f : dir.listFiles()) {
            if(f.toString().endsWith(".c")) {
              if (verbose || Preferences.getBoolean("build.verbose"))  System.out.println("  " + dir.getName() + ":" + f.getName());
              writer.print(" " + f.toString());
            }
          }
        
        //writer.print(" " + (String) includePaths.get(i) + File.separator + "library.c");
      }
      writer.println("");
      writer.println("include platform.mk");
      writer.close();
    } catch(Exception e) {
      e.printStackTrace();
    }
    
    
    

/*   
    // 1.2 clean old make info
    sketch.setCompilingProgress(30);
    List baseClean = new ArrayList(Arrays.asList(new String[] {
      "make", "-C",  buildPath, "clean"
    }));

    execAsynchronously(baseClean, null);
    System.out.println("now: " + buildPath + " is shiny");
*/

    sketch.setCompilingProgress(40);

    // 1.75 compile the sketch (straight from a makefile)

    sketch.setCompilingProgress(50);
    List baseMake;
    if (verbose || Preferences.getBoolean("build.verbose")) {
      baseMake = new ArrayList(Arrays.asList(new String[] {
        "make", "-C", buildPath 
      }));
    } else {    
      baseMake = new ArrayList(Arrays.asList(new String[] {
        "make", "-s", "-C", buildPath 
      }));
    }
    
    execAsynchronously(baseMake, null);

    sketch.setCompilingProgress(90);
    
    return true;

/*    
    // 2. upload the sketch

    sketch.setCompilingProgress(70);
    List baseUpload = new ArrayList(Arrays.asList(new String[] {
      "make", "-C",  buildPath, "program"
    }));

    execAsynchronously(baseUpload, null);

    sketch.setCompilingProgress(80);
   
    System.out.println("check the build folder at: " + buildPath);
    
    sketch.setCompilingProgress(90);
*/   
  }


  private List<File> compileFiles(String avrBasePath,
                                  String buildPath, List<File> includePaths,
                                  List<File> sSources, 
                                  List<File> cSources, List<File> cppSources,
                                  Map<String, String> boardPreferences)
    throws RunnerException {

    List<File> objectPaths = new ArrayList<File>();
    
    for (File file : sSources) {
      String objectPath = buildPath + File.separator + file.getName() + ".o";
      objectPaths.add(new File(objectPath));
      execAsynchronously(getCommandCompilerS(avrBasePath, includePaths,
                                             file.getAbsolutePath(),
                                             objectPath,
                                             boardPreferences), null);
    }
 		
    for (File file : cSources) {
        String objectPath = buildPath + File.separator + file.getName() + ".o";
        String dependPath = buildPath + File.separator + file.getName() + ".d";
        File objectFile = new File(objectPath);
        File dependFile = new File(dependPath);
        objectPaths.add(objectFile);
        if (is_already_compiled(file, objectFile, dependFile, boardPreferences)) continue;
        execAsynchronously(getCommandCompilerC(avrBasePath, includePaths,
                                               file.getAbsolutePath(),
                                               objectPath,
                                               boardPreferences), null);
    }

    for (File file : cppSources) {
        String objectPath = buildPath + File.separator + file.getName() + ".o";
        String dependPath = buildPath + File.separator + file.getName() + ".d";
        File objectFile = new File(objectPath);
        File dependFile = new File(dependPath);
        objectPaths.add(objectFile);
        if (is_already_compiled(file, objectFile, dependFile, boardPreferences)) continue;
        execAsynchronously(getCommandCompilerCPP(avrBasePath, includePaths,
                                                 file.getAbsolutePath(),
                                                 objectPath,
                                                 boardPreferences), null);
    }
    
    return objectPaths;
  }

  private boolean is_already_compiled(File src, File obj, File dep, Map<String, String> prefs) {
    boolean ret=true;
    try {
      //System.out.println("\n  is_already_compiled: begin checks: " + obj.getPath());
      if (!obj.exists()) return false;  // object file (.o) does not exist
      if (!dep.exists()) return false;  // dep file (.d) does not exist
      long src_modified = src.lastModified();
      long obj_modified = obj.lastModified();
      if (src_modified >= obj_modified) return false;  // source modified since object compiled
      if (src_modified >= dep.lastModified()) return false;  // src modified since dep compiled
      BufferedReader reader = new BufferedReader(new FileReader(dep.getPath()));
      String line;
      boolean need_obj_parse = true;
      while ((line = reader.readLine()) != null) {
        if (line.endsWith("\\")) {
          line = line.substring(0, line.length() - 1);
        }
        line = line.trim();
        if (line.length() == 0) continue; // ignore blank lines
        if (need_obj_parse) {
          // line is supposed to be the object file - make sure it really is!
          if (line.endsWith(":")) {
            line = line.substring(0, line.length() - 1);
            String objpath = obj.getCanonicalPath();
            File linefile = new File(line);
            String linepath = linefile.getCanonicalPath();
            //System.out.println("  is_already_compiled: obj =  " + objpath);
            //System.out.println("  is_already_compiled: line = " + linepath);
            if (objpath.compareTo(linepath) == 0) {
              need_obj_parse = false;
              continue;
            } else {
              ret = false;  // object named inside .d file is not the correct file!
              break;
            }
          } else {
            ret = false;  // object file supposed to end with ':', but didn't
            break;
          }
        } else {
          // line is a prerequisite file
          File prereq = new File(line);
          if (!prereq.exists()) {
            ret = false;  // prerequisite file did not exist
            break;
          }
          if (prereq.lastModified() >= obj_modified) {
            ret = false;  // prerequisite modified since object was compiled
            break;
          }
          //System.out.println("  is_already_compiled:  prerequisite ok");
        }
      }
      reader.close();
    } catch (Exception e) {
      return false;  // any error reading dep file = recompile it
    }
    if (ret && (verbose || Preferences.getBoolean("build.verbose"))) {
      System.out.println("  Using previously compiled: " + obj.getPath());
    }
    return ret;
  }


  private ArrayList execExternTool(List commandList, File path) throws RunnerException {
    Runtime rt = Runtime.getRuntime();
    
    String[] command = new String[commandList.size()];
    commandList.toArray(command);

    // Print command
    if (verbose) {
      for(int j = 0; j < command.length; j++) {
        System.out.print(command[j] + " ");
      }
      System.out.println();
    }

    Process proc;
    int result;

    // Run process
    try {
      if(path == null) {
        proc = rt.exec(command);
      } else {
        proc = rt.exec(command, null, path);
      }

    } catch (IOException e) {
      RunnerException re = new RunnerException(e.getMessage());
      re.hideStackTrace();
      throw re;
    }
    
    // Streams
    BufferedReader stdInput = new BufferedReader(new InputStreamReader(proc.getInputStream()));
    BufferedReader stdError = new BufferedReader(new InputStreamReader(proc.getErrorStream()));

    // Output to list
    ArrayList output = new ArrayList();
    String s = null;
    try {
      while ((s = stdInput.readLine()) != null) {
        output.add(s);
      }
    } catch(IOException e) {
      RunnerException re = new RunnerException(_("Error compiling."));
    }
    
    // Errors to System.out
    try {
      while ((s = stdError.readLine()) != null) {
        System.out.println(s);
      }
    } catch(IOException e) {
      RunnerException re = new RunnerException(_("Error compiling."));
    }
    
    // Wait for completion
    try {
      result = proc.waitFor();
    } catch (InterruptedException ignored) {
      RunnerException re = new RunnerException(_("Error compiling."));
      re.hideStackTrace();
      throw re;
    }
    
    // Check result
    if (result != 0) {
      RunnerException re = new RunnerException(_("Error compiling."));
      re.hideStackTrace();
      throw re;
    }

    return output;
  }

  boolean firstErrorFound;
  boolean secondErrorFound;

  /**
   * Either succeeds or throws a RunnerException fit for public consumption.
   */
  private void execAsynchronously(List commandList, File path) throws RunnerException {
    String[] command = new String[commandList.size()];
    commandList.toArray(command);
    int result = 0;
    
    if (verbose || Preferences.getBoolean("build.verbose")) {
      for(int j = 0; j < command.length; j++) {
        System.out.print(command[j] + " ");
      }
      System.out.println();
    }

    firstErrorFound = false;  // haven't found any errors yet
    secondErrorFound = false;

    Process process;
    
    try {
      if(path == null) {
        process = Runtime.getRuntime().exec(command);
      } else {
        process = Runtime.getRuntime().exec(command, null, path);
      }
    } catch (IOException e) {
      RunnerException re = new RunnerException(e.getMessage());
      re.hideStackTrace();
      throw re;
    }

    MessageSiphon in = new MessageSiphon(process.getInputStream(), this);
    MessageSiphon err = new MessageSiphon(process.getErrorStream(), this);

    // wait for the process to finish.  if interrupted
    // before waitFor returns, continue waiting
    boolean compiling = true;
    while (compiling) {
      try {
        in.join();
        err.join();
        result = process.waitFor();
        //System.out.println("result is " + result);
        compiling = false;
      } catch (InterruptedException ignored) { }
    }

    // an error was queued up by message(), barf this back to compile(),
    // which will barf it back to Editor. if you're having trouble
    // discerning the imagery, consider how cows regurgitate their food
    // to digest it, and the fact that they have five stomaches.
    //
    //System.out.println("throwing up " + exception);
    if (exception != null) { throw exception; }

    if (result > 1) {
      // a failure in the tool (e.g. unable to locate a sub-executable)
      System.err.println(
	  I18n.format(_("{0} returned {1}"), command[0], result));
    }

    if (result != 0) {
      RunnerException re = new RunnerException(_("Error compiling."));
      re.hideStackTrace();
      throw re;
    }
  }


  /**
   * Part of the MessageConsumer interface, this is called
   * whenever a piece (usually a line) of error message is spewed
   * out from the compiler. The errors are parsed for their contents
   * and line number, which is then reported back to Editor.
   */
  public void message(String s) {
    int i;

    // remove the build path so people only see the filename
    // can't use replaceAll() because the path may have characters in it which
    // have meaning in a regular expression.
    if (!verbose) {
      String m = s.trim();
      if(m.indexOf("cannot find entry symbol Reset_Handler; defaulting to 00000cf8") != -1) {
        return;
      }
      while ((i = s.indexOf(buildPath + File.separator)) != -1) {
        s = s.substring(0, i) + s.substring(i + (buildPath + File.separator).length());
      }
    }


    // look for error line, which contains file name, line number,
    // and at least the first line of the error message
    String errorFormat = "([\\w\\d_]+.\\w+):(\\d+):\\s*error:\\s*(.*)\\s*";
    String[] pieces = PApplet.match(s, errorFormat);

//    if (pieces != null && exception == null) {
//      exception = sketch.placeException(pieces[3], pieces[1], PApplet.parseInt(pieces[2]) - 1);
//      if (exception != null) exception.hideStackTrace();
//    }

    if (pieces != null) {
      String error = pieces[3], msg = "";
      
      if (pieces[3].trim().equals("SPI.h: No such file or directory")) {
        error = _("Please import the SPI library from the Sketch > Import Library menu.");
        msg = _("\nAs of Arduino 0019, the Ethernet library depends on the SPI library." +
              "\nYou appear to be using it or another library that depends on the SPI library.\n\n");
      }
      
      if (pieces[3].trim().equals("'BYTE' was not declared in this scope")) {
        error = _("The 'BYTE' keyword is no longer supported.");
        msg = _("\nAs of Arduino 1.0, the 'BYTE' keyword is no longer supported." +
              "\nPlease use Serial.write() instead.\n\n");
      }
      
      if (pieces[3].trim().equals("no matching function for call to 'Server::Server(int)'")) {
        error = _("The Server class has been renamed EthernetServer.");
        msg = _("\nAs of Arduino 1.0, the Server class in the Ethernet library " +
              "has been renamed to EthernetServer.\n\n");
      }
      
      if (pieces[3].trim().equals("no matching function for call to 'Client::Client(byte [4], int)'")) {
        error = _("The Client class has been renamed EthernetClient.");
        msg = _("\nAs of Arduino 1.0, the Client class in the Ethernet library " +
              "has been renamed to EthernetClient.\n\n");
      }
      
      if (pieces[3].trim().equals("'Udp' was not declared in this scope")) {
        error = _("The Udp class has been renamed EthernetUdp.");
        msg = _("\nAs of Arduino 1.0, the Udp class in the Ethernet library " +
              "has been renamed to EthernetUdp.\n\n");
      }
      
      if (pieces[3].trim().equals("'class TwoWire' has no member named 'send'")) {
        error = _("Wire.send() has been renamed Wire.write().");
        msg = _("\nAs of Arduino 1.0, the Wire.send() function was renamed " +
              "to Wire.write() for consistency with other libraries.\n\n");
      }
      
      if (pieces[3].trim().equals("'class TwoWire' has no member named 'receive'")) {
        error = _("Wire.receive() has been renamed Wire.read().");
        msg = _("\nAs of Arduino 1.0, the Wire.receive() function was renamed " +
              "to Wire.read() for consistency with other libraries.\n\n");
      }

      if (pieces[3].trim().equals("'Mouse' was not declared in this scope")) {
        error = _("'Mouse' only supported on the Arduino Leonardo");
        //msg = _("\nThe 'Mouse' class is only supported on the Arduino Leonardo.\n\n");
      }
      
      if (pieces[3].trim().equals("'Keyboard' was not declared in this scope")) {
        error = _("'Keyboard' only supported on the Arduino Leonardo");
        //msg = _("\nThe 'Keyboard' class is only supported on the Arduino Leonardo.\n\n");
      }
      
      RunnerException e = null;
      if (!sketchIsCompiled) {
        // Place errors when compiling the sketch, but never while compiling libraries
        // or the core.  The user's sketch might contain the same filename!
        e = sketch.placeException(error, pieces[1], PApplet.parseInt(pieces[2]) - 1);
      }

      // replace full file path with the name of the sketch tab (unless we're
      // in verbose mode, in which case don't modify the compiler output)
      if (e != null && !verbose) {
        SketchCode code = sketch.getCode(e.getCodeIndex());
        String fileName = (code.isExtension("ino") || code.isExtension("pde")) ? code.getPrettyName() : code.getFileName();
        int lineNum = e.getCodeLine() + 1;
        s = fileName + ":" + lineNum + ": error: " + pieces[3] + msg;        
      }
            
      if (exception == null && e != null) {
        exception = e;
        exception.hideStackTrace();
      }      
    }
    
    System.err.print(s);
  }

//XXX patches to make the arm compilation quickly

public static void copyFolder(File src, File dest) throws IOException {
	if(src.isDirectory()) {
		// If directory not exists, create it
		if(!dest.exists()) {
		   dest.mkdir();
		}
		// List all the directory contents
		String files[] = src.list();

		for (String file : files) {
		   // Construct the src and dest file structure
		   File srcFile = new File(src, file);
		   File destFile = new File(dest, file);
		   // Recursive copy
		   copyFolder(srcFile,destFile);
		}
	} else if(!dest.exists() || src.lastModified() > dest.lastModified()) {
		// If file, then copy it
		// Use bytes stream to support all file types
		InputStream in = new FileInputStream(src);
		OutputStream out = new FileOutputStream(dest); 

		byte[] buffer = new byte[1024];

		int length;
		// Copy the file content in bytes 
		while ((length = in.read(buffer)) > 0){
		   out.write(buffer, 0, length);
		}
		
		in.close();
		out.close();
		dest.setLastModified(src.lastModified());


	}
}

//XXX

public static void copyFile(String src, String dest)
    	throws IOException{
    InputStream in = new FileInputStream(src);
    OutputStream out = new FileOutputStream(dest);
    byte[] buf = new byte[1024];
    int len;
    while ((len = in.read(buf)) > 0) {
       out.write(buf, 0, len);
    }
    in.close();
    out.close(); 
}
  /////////////////////////////////////////////////////////////////////////////

  static private List getCommandCompilerS(String avrBasePath, List includePaths,
    String sourceName, String objectName, Map<String, String> boardPreferences) {
    List baseCommandCompiler = new ArrayList(Arrays.asList(new String[] {
      avrBasePath + "avr-gcc",
      "-c", // compile, don't link
      "-g", // include debugging info (so errors include line numbers)
      "-x","assembler-with-cpp",
      "-mmcu=" + boardPreferences.get("build.mcu"),
      "-DF_CPU=" + boardPreferences.get("build.f_cpu"),      
      "-DARDUINO=" + Base.REVISION,
      "-DUSB_VID=" + boardPreferences.get("build.vid"),
      "-DUSB_PID=" + boardPreferences.get("build.pid"),
    }));

    for (int i = 0; i < includePaths.size(); i++) {
      baseCommandCompiler.add("-I" + (String) includePaths.get(i));
    }

    baseCommandCompiler.add(sourceName);
    baseCommandCompiler.add("-o"+ objectName);

    return baseCommandCompiler;
  }

  
  static private List getCommandCompilerC(String avrBasePath, List includePaths,
    String sourceName, String objectName, Map<String, String> boardPreferences) {

    List baseCommandCompiler = new ArrayList(Arrays.asList(new String[] {
      avrBasePath + "avr-gcc",
      "-c", // compile, don't link
      "-g", // include debugging info (so errors include line numbers)
      "-Os", // optimize for size
      Preferences.getBoolean("build.verbose") ? "-Wall" : "-w", // show warnings if verbose
      "-ffunction-sections", // place each function in its own section
      "-fdata-sections",
      "-mmcu=" + boardPreferences.get("build.mcu"),
      "-DF_CPU=" + boardPreferences.get("build.f_cpu"),
      "-MMD", // output dependancy info
      "-DUSB_VID=" + boardPreferences.get("build.vid"),
      "-DUSB_PID=" + boardPreferences.get("build.pid"),
      "-DARDUINO=" + Base.REVISION, 
    }));
		
    for (int i = 0; i < includePaths.size(); i++) {
      baseCommandCompiler.add("-I" + (String) includePaths.get(i));
    }

    baseCommandCompiler.add(sourceName);
    baseCommandCompiler.add("-o");
    baseCommandCompiler.add(objectName);

    return baseCommandCompiler;
  }
	
	
  static private List getCommandCompilerCPP(String avrBasePath,
    List includePaths, String sourceName, String objectName,
    Map<String, String> boardPreferences) {
    
    List baseCommandCompilerCPP = new ArrayList(Arrays.asList(new String[] {
      avrBasePath + "avr-g++",
      "-c", // compile, don't link
      "-g", // include debugging info (so errors include line numbers)
      "-Os", // optimize for size
      Preferences.getBoolean("build.verbose") ? "-Wall" : "-w", // show warnings if verbose
      "-fno-exceptions",
      "-ffunction-sections", // place each function in its own section
      "-fdata-sections",
      "-mmcu=" + boardPreferences.get("build.mcu"),
      "-DF_CPU=" + boardPreferences.get("build.f_cpu"),
      "-MMD", // output dependancy info
      "-DUSB_VID=" + boardPreferences.get("build.vid"),
      "-DUSB_PID=" + boardPreferences.get("build.pid"),      
      "-DARDUINO=" + Base.REVISION,
    }));

    for (int i = 0; i < includePaths.size(); i++) {
      baseCommandCompilerCPP.add("-I" + (String) includePaths.get(i));
    }

    baseCommandCompilerCPP.add(sourceName);
    baseCommandCompilerCPP.add("-o");
    baseCommandCompilerCPP.add(objectName);

    return baseCommandCompilerCPP;
  }



  /////////////////////////////////////////////////////////////////////////////

  static private void createFolder(File folder) throws RunnerException {
    if (folder.isDirectory()) return;
    if (!folder.mkdir())
      throw new RunnerException("Couldn't create: " + folder);
  }

  /**
   * Given a folder, return a list of the header files in that folder (but
   * not the header files in its sub-folders, as those should be included from
   * within the header files at the top-level).
   */
  static public String[] headerListFromIncludePath(String path) throws IOException {
    FilenameFilter onlyHFiles = new FilenameFilter() {
      public boolean accept(File dir, String name) {
        return name.endsWith(".h");
      }
    };

    String[] list = (new File(path)).list(onlyHFiles);
    if (list == null) {
      throw new IOException();
    }
    return list;
  }
  
  static public ArrayList<File> findFilesInPath(String path, String extension,
                                                boolean recurse) {
    return findFilesInFolder(new File(path), extension, recurse);
  }
  
  static public ArrayList<File> findFilesInFolder(File folder, String extension,
                                                  boolean recurse) {
    ArrayList<File> files = new ArrayList<File>();
    
    if (folder.listFiles() == null) return files;
    
    for (File file : folder.listFiles()) {
      if (file.getName().startsWith(".")) continue; // skip hidden files
      
      if (file.getName().endsWith("." + extension))
        files.add(file);
        
      if (recurse && file.isDirectory()) {
        files.addAll(findFilesInFolder(file, extension, true));
      }
    }
    
    return files;
  }
}
