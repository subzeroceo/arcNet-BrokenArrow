#!/usr/bin/env python
# prepare content for SDK

import shutil, os, stat

media = '../../../../../media-sdk'
media = os.path.abspath( media )

try:
    shutil.rmtree( 'arcNet' )
except:
    print 'Could not remove arcNet'
    pass

# copy source from list
f = open( 'source.list' )
l = [ s[:-1] for s in f.readlines() ]
f.close()
for p in l:
    sp = os.path.join( '../../..', p )
    dp = os.path.join( 'arcNet/src', p )
    try:
        os.makedirs( os.path.dirname( dp ) )
    except:
        pass
    print 'cp ' + sp + ' -> ' + dp
    shutil.copy( sp, dp )

# copy explicit media content over
for root, dirs, files in os.walk( media ):
    if '.svn' in dirs:
        dirs.remove( '.svn' )
    for f in files:
        sp = os.path.join( root, f )
        dp = os.path.join( 'arcNet', sp[ len( media ) + 1: ] )
        try:
            os.makedirs( os.path.dirname( dp ) )
        except:
            pass
        print 'cp ' + sp + ' -> ' + dp
        shutil.copy( sp, dp )

def makewritable( path ):
    for root, dirs, files in os.walk( path ):
        for f in files:
            os.chmod( os.path.join( root, f ), stat.S_IWRITE )

# cleanup '.svn'
for root, dirs, files in os.walk( 'arcNet' ):
    if '.svn' in dirs:
        print 'remove ' + os.path.join( root, '.svn' )
        # SVN sets readonly on some files, which causes rmtree failure on win32
        makewritable( os.path.join( root, '.svn' ) )
        shutil.rmtree( os.path.join( root, '.svn' ) )
        dirs.remove( '.svn' )
