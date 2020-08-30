#	$NetBSD: bsd.subdir.mk,v 1.47 2004/01/29 01:48:45 lukem Exp $
#	@(#)bsd.subdir.mk	8.1 (Berkeley) 6/8/93

.include <bsd.init.mk>

.for dir in ${SUBDIR}
.if exists(${dir}.${MACHINE})
__REALSUBDIR+=${dir}.${MACHINE}
.else
__REALSUBDIR+=${dir}
.endif
.endfor

__recurse: .USE
	@targ=${.TARGET:C/-.*$//};dir=${.TARGET:C/^[^-]*-//};		\
	case "$$dir" in /*)											\
		echo "$$targ ===> $$dir";								\
		cd "$$dir";												\
		${MAKE} "_THISDIR_=$$dir/" $$targ;						\
		;;														\
	*)															\
		echo "$$targ ===> ${_THISDIR_}$$dir";					\
		cd "${.CURDIR}/$$dir";									\
		${MAKE} "_THISDIR_=${_THISDIR_}$$dir/" $$targ;			\
		;;														\
	esac

.if make(cleandir)
__RECURSETARG=	${TARGETS:Nclean}
clean:
.else
__RECURSETARG=	${TARGETS}
.endif

# for obscure reasons, we can't do a simple .if ${dir} == ".WAIT"
# but have to assign to __TARGDIR first.
.for targ in ${__RECURSETARG}
.for dir in ${__REALSUBDIR}
__TARGDIR := ${dir}
.if ${__TARGDIR} == ".WAIT"
SUBDIR_${targ} += .WAIT
.elif !commands(${targ}-${dir})
${targ}-${dir}: .PHONY .MAKE __recurse
SUBDIR_${targ} += ${targ}-${dir}
.endif
.endfor
.if defined(__REALSUBDIR)
subdir-${targ}: .PHONY ${SUBDIR_${targ}}
${targ}: subdir-${targ}
.endif
.endfor

${TARGETS}:	# ensure existence