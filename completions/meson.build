if get_option('zsh-completions')
	zsh_files = files(
		'zsh/_swayng',
		'zsh/_swayngmsg',
	)
	zsh_install_dir = join_paths(datadir, 'zsh', 'site-functions')

	install_data(zsh_files, install_dir: zsh_install_dir)
endif

if get_option('bash-completions')
	bash_comp = dependency('bash-completion', required: false)

	bash_files = files(
		'bash/swayng',
		'bash/swayngmsg',
	)

	if get_option('swayngbar')
		bash_files += files('bash/swayngbar')
	endif

	if bash_comp.found()
		bash_install_dir = bash_comp.get_variable(
			pkgconfig: 'completionsdir',
			pkgconfig_define: ['datadir', datadir]
		)
	else
		bash_install_dir = join_paths(datadir, 'bash-completion', 'completions')
	endif

	install_data(bash_files, install_dir: bash_install_dir)
endif

if get_option('fish-completions')
	fish_comp = dependency('fish', required: false)

	fish_files = files(
		'fish/swayng.fish',
		'fish/swayngmsg.fish',
	)

	if get_option('swayngnag')
		fish_files += files('fish/swayngnag.fish')
	endif

	if fish_comp.found()
		fish_install_dir = fish_comp.get_variable(
			pkgconfig: 'completionsdir',
			pkgconfig_define: ['datadir', datadir]
		)
	else
		fish_install_dir = join_paths(datadir, 'fish', 'vendor_completions.d')
	endif

	install_data(fish_files, install_dir: fish_install_dir)
endif
