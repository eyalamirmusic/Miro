// In-process "bridge" transport.
//
// A Transport (defined in the generated schema.bridge module) is the
// pluggable seam between a typed client (makeBackend) and whatever
// actually carries the request — an HTTP fetch, a WebSocket, an IPC
// pipe to a native host, etc. This example skips the wire entirely
// and routes every invoke() straight into the generated dispatch()
// function, which in turn calls one of the handlers defined in
// handlers.ts. The result is a fully typed end-to-end call that lives
// entirely inside the Node process — exactly the setup you'd use in
// unit tests, or as a development stub before the real transport is
// hooked up.

import type { Transport } from './generated/schema.bridge';
import { dispatch } from './generated/schema.handlers';
import { handlers } from './handlers';

export const localTransport: Transport = {
    async invoke(command: string, payload: unknown): Promise<unknown>
    {
        console.log(`[transport] invoke '${command}'`);
        return dispatch(handlers, command, payload);
    },
};
