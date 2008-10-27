#!/usr/bin/env ruby

IO.foreach(ARGV[0]) do |line| if line =~ /Rsp/
    line.scan(/(\w+).*( \d+ ).*/) { |val1, val2| printf  "||%s||%.3f||\n", val1, 1000000/val2.to_s.to_f }
    end
end
