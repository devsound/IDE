#!/usr/bin/env ruby

require 'optparse'

class Linkdep
  def initialize
    @undef_syms = Hash.new{|h,k| h[k]=[]}
    @def_syms = {}
  end

  def symvar(sym)
    "_syms.#{sym}"
  end

  def get_syms(obj)
    nm = ENV["NM"] || "arm-none-eabi-nm"
    IO.popen([nm] + %w{-A -P -g} + [obj]) do |output|
      yield output
    end
  end

  def process(file, include_path=true)
    get_syms(file) do |ls|
      file = File.basename(file) if !include_path
      # add an empty entry to def_syms so that we can output the link
      # info for this file, even if it doesn't have any symbols.

      @def_syms[file] = []

      ls.each do |l|
        _, _, sym, type = l.match(/^(.*): ([^[:space:]]*) (.) .*/).to_a
        case type
        when 'U'
          @undef_syms[file] << sym
        else
          @def_syms[file] << sym
        end
      end
    end
  end

  def write(out)
    @def_syms.each do |f, syms|
      out.puts "#{symvar(f)}=\t#{f}"
      syms.each do |s|
        out.puts "#{symvar(s)}+=\t${#{symvar(f)}}"
      end
    end

    @undef_syms.each do |f, syms|
      vars = syms.map{|s| "$${#{symvar(s)}}"}
      out.puts "#{symvar(f)}+=\t#{vars.join(' ')}"
      out.puts "_syms_files+=\t#{symvar(f)}"
    end
  end
end

if $0 == __FILE__
  output = nil
  include_path = true
  OptionParser.new do |opts|
    opts.on("-o", "--output FILE") do |f|
      output = f
    end
    opts.on("--[no-]path") do |v|
      include_path = v
    end
  end.parse!

  ld = Linkdep.new
  ARGV.each do |f|
    ld.process f, include_path
  end

  if output
    File.open(output, 'w') do |o|
      ld.write(o)
    end
  else
    ld.write($stdout)
  end
end
