#!/usr/bin/env ruby

IO.foreach(ARGV[0]) do |wikiline| 
    wikiline.scan(/(\w+).*/) do |method| 
        found = false
        IO.foreach(ARGV[1]) do |testline|
            if !found && testline.rindex(method.to_s + ".")
                found = true
                testline.scan(/.* (\d+) .*/) { |result| printf "%s%.3f||\n",wikiline.chomp, 1000000/result.to_s.to_f }
            end
        end

        if !found
            print wikiline.chomp, "NA||\n"
        end
    end
end
