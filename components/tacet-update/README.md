# tacet-update

Thin wrapper over `bootc` with a greenboot-style health check (spec §4.6).
Milestone M2. **Not started** — M1 must be demonstrated on hardware first.

Already in place for it: `bootc-fetch-apply-updates.timer` is masked in the
image so nothing updates without this component; `tacet-session.service` is
the unit the health check will query.
