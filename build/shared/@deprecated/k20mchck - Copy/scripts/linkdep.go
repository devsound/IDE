package main

import "fmt"
import "flag"
import "os"
import "os/exec"
import "path"
import "regexp"
import "strings"

var def_syms map[string][]string = make(map[string][]string)
var undef_syms map[string][]string = make(map[string][]string)

func handleErr(e error) {
	if e != nil {
		panic(e)
	}
}

func symvar(sym string) string {
	return "_syms." + sym
}

func get_syms(obj string) string {
	nm := os.Getenv("NM")
	if nm == "" {
		nm = "arm-none-eabi-nm"
	}
	syms, err := exec.Command(nm, "-A", "-P", "-g", obj).Output()
	handleErr(err)
	return string(syms)
}

func process(file string, includePath bool) {
	ls := get_syms(file)
	if !includePath {
		file = path.Base(file)
	}
	def := make([]string, 0)
	undef := make([]string, 0)
	re := regexp.MustCompile("^(.*): ([^[:space:]]*) (.) .*")
	for _, l := range strings.Split(ls, "\n") {
		matches := re.FindStringSubmatch(l)
		sym := matches[2]
		t := matches[3]
		if t == "U" {
			undef = append(undef, sym)
		} else {
			def = append(def, sym)
		}
	}
	def_syms[file] = def
	undef_syms[file] = undef
}

func write(output *os.File) {
	for f, syms := range def_syms {
		output.WriteString(fmt.Sprintf("%s=\t%s\n", symvar(f), f))
		for _, s := range(syms) {
			output.WriteString(fmt.Sprintf("%s+=\t%s\n", symvar(s), symvar(f)))
		}
	}
	for f, syms := range undef_syms {
		output.WriteString(fmt.Sprintf("%s+=\t", symvar(f)))
		for i, s := range(syms) {
			if i > 0 {
				output.WriteString(" ")
			}
			output.WriteString(fmt.Sprintf("$${%s}", symvar(s)))
		}
		output.WriteString("\n")
		output.WriteString(fmt.Sprintf("_syms_files+=\t%s", symvar(f)))
	}
}

func main() {
	outputPtr := flag.String("o", "", "output file")
	flag.Bool("path", false, "path")
	noIncludePathPtr := flag.Bool("no-path", false, "no-path")
	flag.Parse()
	includePath := !*noIncludePathPtr

	fmt.Printf("Hello, world.\n%s, %b", *outputPtr, includePath)

	for _, f := range flag.Args() {
		process(f, includePath)
		fmt.Printf("arg: %s\n", f)
	}

	output := os.Stdout
	if *outputPtr != "" {
		var err error
		output, err = os.Create(*outputPtr)
		handleErr(err)
		defer output.Close()
	}
	write(output)
}
