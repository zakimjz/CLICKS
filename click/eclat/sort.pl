#!/usr/grads/bin/perl5

sub isetsub(){
    #print "$a --- $b\n";
    @la = split $a;
    @lb = split $b;
    if ($#la > $#lb){
        return 1;
    }
    elsif ($#la < $#lb){
        return -1;
    }
    else{
        for ($i=0; $i < $#la+1; $i++){
            if ($la[$i] > $lb[$i]){
                return 1;
            }
            elsif ($la[$i] < $lb[$i]){
                return -1;
            }
        }
        return 0;
    }
}

$cnt = 0;
$maxl = 0;
while(<>){
    chop; split;
    @ll = "";
    if ($#_ > $maxl){
        $maxl = $#_;
    }
    $len[$#_]++;

    for ($i=0; $i < $#_+1; $i++){
        @ll[$i] = $_[$i];
    }
    $tt[$cnt] = join (" ", @ll);
    print $tt[$cnt], "\n";
    $cnt++;
}

#print "MAXL $maxl\n";
#for($i=0; $i <= $maxl; $i++){
#    print $i, " ", $len[$i], "\n";
#}

@stt = sort (isetsub($a,$b), @tt);

for ($i=0; $i < $cnt; $i++){
    print $stt[$i], "\n";
}
