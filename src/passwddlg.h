/* passwddlg.h - The GNU Privacy Assistant
 *      Copyright (C) 2002, Miguel Coca.
 *
 * This file is part of GPA
 *
 * GPA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GPA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* The "new passphrase" dialog. It's really a GPGME passphrase callback. */

#ifndef PASSWDDLG_H
#define PASSWDDLG_H

gpg_error_t gpa_change_passphrase_dialog_run (void *hook, 
					      const char *uid_hint,
					      const char *passphrase_info, 
					      int prev_was_bad, int fd);

#endif
