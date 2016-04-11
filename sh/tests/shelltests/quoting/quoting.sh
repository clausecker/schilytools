#! /bin/sh

# Read test core functions
. ../../common/test-common

docommand q1 "$SHELL quoting-1" 0 '<bastename /usr/Acroread/Reader>\n' ""
docommand q2 "$SHELL quoting-2" 0 '<bastename "/usr/Acroread/Reader">\n' ""
docommand q3 "$SHELL quoting-3" 0 '<bastename "/usr/Acroread/Reader">\n' ""
docommand q4 "$SHELL quoting-4" 0 '<bastename "/usr/Acroread/Reader">\n' ""
docommand q5 "$SHELL quoting-5" 0 '<ba\s	ename /usr/Acroread/Reader>\n' ""
docommand q6 "$SHELL quoting-6" 0 '<ba\s	ename "/usr/Acroread/Reader">\n' ""
docommand q7 "$SHELL quoting-7" 0 '<ba\s	ename "/usr/Acroread/Reader">\n' ""
docommand q8 "$SHELL quoting-8" 0 '<ba\s	ename "/usr/Acroread/Reader">\n' ""
docommand q9 "$SHELL quoting-9" 0 '<ba\s	ename /usr/Acroread/Reader>\n' ""
docommand q10 "$SHELL quoting-10" 0 '<"bastename" "/usr/Acroread/Reader">\n' ""
docommand q11 "$SHELL quoting-11" 0 '<"bastename" "/usr/Acroread/Reader">\n' ""
docommand q12 "$SHELL quoting-12" 0 '<"bastename" "/usr/Acroread/Reader">\n' ""

docommand q20 "$SHELL bquote" 0 '<Reader>\n' ""

success
