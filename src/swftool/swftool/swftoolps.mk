
swftoolps.dll: dlldata.obj swftool_p.obj swftool_i.obj
	link /dll /out:swftoolps.dll /def:swftoolps.def /entry:DllMain dlldata.obj swftool_p.obj swftool_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del swftoolps.dll
	@del swftoolps.lib
	@del swftoolps.exp
	@del dlldata.obj
	@del swftool_p.obj
	@del swftool_i.obj
