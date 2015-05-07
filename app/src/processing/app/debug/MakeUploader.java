/* -*- mode: jde; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
  MakeUploader - uploader implementation using makefile
  Part of the Arduino project - http://www.arduino.cc/

  Copyright (c) 2004-05
  Hernando Barragan

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
  
  $Id$
*/

package processing.app.debug;

import processing.app.Base;
import processing.app.Preferences;
import processing.app.Sketch;
import processing.app.SketchCode;
import processing.core.*;
import processing.app.I18n;
import static processing.app.I18n._;

import javax.swing.*;

import java.io.*;
import java.util.*;
import java.util.zip.*;


public class MakeUploader extends Uploader  {
  String buildPath;
  String primaryClassName;
  Sketch sketch;

  public MakeUploader() {
  }

  public boolean uploadUsingPreferences(Sketch sketch, String buildPath, String className, boolean usingProgrammer)
  throws RunnerException {
    this.sketch = sketch;
    this.buildPath = buildPath;
    this.primaryClassName = primaryClassName;
    String uploadPort = Preferences.get("serial.port");

    String progMode = Preferences.get("progmode");
    if(progMode == null) {
    	RunnerException re = new RunnerException(_("No programming mode selected; plase choose a mode from the Tools > Programming mode menu."));
      re.hideStackTrace();
      throw re;
    }

    String tools     = Base.getArmBasePath();
    String binTools = tools + "bin" + File.separator;

    if(progMode.equals("Serial")) {
      List baseUpload;
      if (verbose || Preferences.getBoolean("upload.verbose")) {
        baseUpload = new ArrayList(Arrays.asList(new String[] {
          "make", "swd-flash", "MCHCKADAPTER=name=mchck:dev=" + uploadPort, "-C",  this.buildPath
        }));
      } else {    
        baseUpload = new ArrayList(Arrays.asList(new String[] {
          "make", "swd-flash", "MCHCKADAPTER=name=mchck:dev=" + uploadPort, "-s", "-C",  this.buildPath
        }));
      }
      execAsynchronously(baseUpload);
    } else if(progMode.equals("DFU")) {
      List baseUpload;
      if (verbose || Preferences.getBoolean("upload.verbose")) {
        baseUpload = new ArrayList(Arrays.asList(new String[] {
          binTools + "dfu-util", "-dDE50:0002", "-a0", "-i0", "-D" + this.buildPath + File.separator + "sketch.bin"
        }));
      } else {    
        baseUpload = new ArrayList(Arrays.asList(new String[] {
          binTools + "dfu-util", "-dDE50:0002", "-a0", "-i0", "-D" + this.buildPath + File.separator + "sketch.bin"
        }));
      }
      execAsynchronously(baseUpload);
    }

    

    return true;
  }

  public boolean burnBootloader() throws RunnerException {
    return true;
  }  
  
  /**
   * Either succeeds or throws a RunnerException fit for public consumption.
   */
  private void execAsynchronously(List commandList) throws RunnerException {
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
      process = Runtime.getRuntime().exec(command);
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
      //System.err.println(
      //I18n.format(_("{0} returned {1}"), command[0], result));
      switch(result) {
        case 74:
          System.err.println("Uploading failed - no device found.");
          break;
        default:
          System.err.println(
            I18n.format(_("Uploading failed with unknown error {0}"), result)
          );
      }
    }

    if (result != 0) {
      RunnerException re = new RunnerException(_("Error uploading."));
      re.hideStackTrace();
      throw re;
    }
  }

}
