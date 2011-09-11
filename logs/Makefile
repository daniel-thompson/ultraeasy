all :
	cat SnoopyProExport.log \
	| dos2unix \
	| grep payloadbytes \
	| grep -v 's><' \
	| sed -e 's:</*payloadbytes>::g' \
	| sed -e 's/[0123456789abcdef][0123456789abcdef][0123456789abcdef][0123456789abcdef][0123456789abcdef][0123456789abcdef]/PC to meter:  &/' \
	| sed -e 's/^02$$/Meter to PC:  02/' \
	> otdms.log
