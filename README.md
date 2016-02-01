org-id-scanner
==============

This program scans Emacs Org mode files creates the .org-id-locations
alist files (the type of file typically stored in the org-id-locations
Emacs variable).  This scanner runs much more quickly than the
Elisp-based scanner defined in org-id-update-id-locations defined in
the org-mode file, org-mode/lisp/org-id.el.  And, it ignores agenda
files which is an optimization needed by org-id-update-id-locations,
due to its slowness. 

Build on Linux via:

    RUN_ARGUMENTS="-o id-alist-work.el -- ~/Plans/Work/Work" make all

Then execute via:

    cd ~/MyOrgDirectory
    ./org-id-scanner.exe -o .org-id-locations -- .
