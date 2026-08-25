package main

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/omrimorgan5-hub/quill-qpm/internal/deps"
	"github.com/omrimorgan5-hub/quill-qpm/internal/manifest"
)

func main() {
	if len(os.Args) < 2 {
		printUsage()
		os.Exit(1)
	}

	cmd := os.Args[1]
	args := os.Args[2:]

	switch cmd {
	case "init":
		runInit(args)
	case "add":
		runAdd(args)
	case "install":
		runInstall(args)
	case "update":
		runUpdate(args)
	case "list", "ls":
		runList(args)
	default:
		fmt.Fprintf(os.Stderr, "unknown command: %s\n", cmd)
		printUsage()
		os.Exit(1)
	}
}

func printUsage() {
	fmt.Println(`qpm — Quill Package Manager

Usage:
  qpm init                Create quill.json in current directory
  qpm add <spec>          Add a dependency (e.g., qpm add omri/math)
  qpm install             Install all dependencies from quill.json
  qpm update              Update all dependencies to latest
  qpm list                List installed dependencies

Dependency spec format:
  user/repo               Short form (uses github, latest)
  user/repo#v1.2.0        Pin to tag or commit
  github:user/repo#abc    Full form with host and ref`)
}

func runInit(args []string) {
	cwd, _ := os.Getwd()
	name := filepath.Base(cwd)

	m := manifest.Manifest{
		Name:    name,
		Version: "0.1.0",
		Main:    "main.qsc",
		Deps:    make(map[string]string),
	}

	if err := manifest.Write("quill.json", m); err != nil {
		fmt.Fprintf(os.Stderr, "error: %v\n", err)
		os.Exit(1)
	}

	fmt.Println("Created quill.json")
}

func runAdd(args []string) {
	if len(args) < 1 {
		fmt.Fprintln(os.Stderr, "usage: qpm add <spec>")
		os.Exit(1)
	}

	spec := args[0]

	m, err := manifest.Read("quill.json")
	if err != nil {
		fmt.Fprintf(os.Stderr, "error reading quill.json: %v\n", err)
		os.Exit(1)
	}

	dep, err := deps.ParseSpec(spec)
	if err != nil {
		fmt.Fprintf(os.Stderr, "error parsing spec: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("Adding %s/%s...\n", dep.User, dep.Repo)

	if err := deps.Install(dep, "modules"); err != nil {
		fmt.Fprintf(os.Stderr, "error installing: %v\n", err)
		os.Exit(1)
	}

	m.Deps[dep.FullName()] = dep.String()

	if err := manifest.Write("quill.json", m); err != nil {
		fmt.Fprintf(os.Stderr, "error writing quill.json: %v\n", err)
		os.Exit(1)
	}

	fmt.Printf("Added %s@%s\n", dep.FullName(), dep.Ref)
}

func runInstall(args []string) {
	m, err := manifest.Read("quill.json")
	if err != nil {
		fmt.Fprintf(os.Stderr, "error reading quill.json: %v\n", err)
		os.Exit(1)
	}

	if len(m.Deps) == 0 {
		fmt.Println("No dependencies to install")
		return
	}

	for spec, raw := range m.Deps {
		dep, err := deps.ParseSpec(raw)
		if err != nil {
			fmt.Fprintf(os.Stderr, "error parsing %s: %v\n", spec, err)
			continue
		}

		fmt.Printf("Installing %s...\n", dep.FullName())
		if err := deps.Install(dep, "modules"); err != nil {
			fmt.Fprintf(os.Stderr, "error installing %s: %v\n", dep.FullName(), err)
			os.Exit(1)
		}
	}

	fmt.Println("Done")
}

func runUpdate(args []string) {
	m, err := manifest.Read("quill.json")
	if err != nil {
		fmt.Fprintf(os.Stderr, "error reading quill.json: %v\n", err)
		os.Exit(1)
	}

	for spec, raw := range m.Deps {
		dep, err := deps.ParseSpec(raw)
		if err != nil {
			fmt.Fprintf(os.Stderr, "error parsing %s: %v\n", spec, err)
			continue
		}

		fmt.Printf("Updating %s...\n", dep.FullName())
		if err := deps.Update(dep, "modules"); err != nil {
			fmt.Fprintf(os.Stderr, "error updating %s: %v\n", dep.FullName(), err)
		}
	}

	fmt.Println("Done")
}

func runList(args []string) {
	m, err := manifest.Read("quill.json")
	if err != nil {
		fmt.Fprintf(os.Stderr, "error reading quill.json: %v\n", err)
		os.Exit(1)
	}

	if len(m.Deps) == 0 {
		fmt.Println("No dependencies")
		return
	}

	fmt.Println("Dependencies:")
	for name, spec := range m.Deps {
		fmt.Printf("  %s => %s\n", name, spec)
	}
}
