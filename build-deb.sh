# ------------------------------------------------------------------------------
#  Copyright (C) 2012, Robert Johansson <rob@raditex.nu>, Raditex Control AB
#  All rights reserved.
# 
#  rSCADA 
#  http://www.rSCADA.se
#  info@raditex.nu
# 
# ------------------------------------------------------------------------------

if [ ! -f Makefile ]; then
	#
	# regenerate automake files
	#
    echo "Running autotools..."

    autoheader \
        && aclocal \
        && libtoolize --ltdl --copy --force \
        && automake --add-missing --copy \
        && autoconf
fi

debuild -i -us -uc -b 

#This is for building a clean 32bit environment because the project is not clearly cross-compilable
#IMPORTANT: You will need to install additional packages (pbuilder debootstrap)
#           Then You will need to create a chrooted 32bit environment to buld the entire project in with:
#           sudo pbuilder create --architecture i386 --distribution focal --debootstrapopts --arch=i386
#           sudo pbuilder build $(NAME)_$(VERSION)-1.dsc
