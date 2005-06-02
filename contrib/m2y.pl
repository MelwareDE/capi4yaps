#!/usr/bin/perl
#	-*- perl -*-
#	Untested mail -> yaps gateway (just an example)
#	(C) by Ulrich Dessauer
#	Create some receiver address, e.g. <nr>@yaps.<yourdomain> and
#	instruct your mailer to forward each mail to this address to
#	this script. Ensure that there is only one receiver per mail.
#
#	To be really useful, the script must be expanded to check for
#	illegal characters in receiver/message (in this case the >'<
#	as this is used as quoting in the system() call) and handling
#	of multiple receivers in one call, etc. Happy Hacking!
#
#	Markus <markus@mail.yezz.de> made some comments and improvments
#	to make this script slowly useful.
#
$yaps = "/usr/local/bin/yaps";
$mailer = "/usr/lib/sendmail -t";
$send_response = 1;
#
$recv = $ARGV[0];
$recv = (split ('@', $recv))[0];
die "No receiver given\n"
	unless $recv;
undef $msg;
undef $from;
while (<STDIN>) {
	chomp;
	if ($_ =~ /^Subject: /i) {
		$msg = $_;
		$msg =~ s/^Subject: //i;
		undef $msg
			if $msg eq "";
	} elsif ($_ =~ /^From: /i) {
		$from = $_;
		$from =~ s/^From: //i;
	}
}
die "No message found\n"
	unless $msg;
die "No sender found\n"
	unless $from;
#
#	Avoid apostroph in command line (otherwise the shell will stumble)
$recv =~ s/\'/\`/g;
$msg =~ s/\'/\`/g;
$from =~ s/\'/\`/g;
$n = system ("$yaps '$recv' '(EMail from $from) $msg'");
die "Unable to send message to $recv\n"
	if $n;
if ($send_response) {
	die "Unable to invoke mailer for response\n"
		unless open (OUT, "|$mailer");
	print OUT "From: yaps-mail\n";
	print OUT "To: $from\n";
	print OUT "Subject: Message sent to $recv\n";
	print OUT "\n";
	print OUT "Your message $msg\nto $recv has been sent\n";
	close (OUT);
}
