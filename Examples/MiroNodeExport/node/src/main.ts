// Demo entry point. Builds a typed client from the generated backend
// factory and the local in-process transport, then calls each command.
// The full stack — client typing, command dispatch, handler bodies —
// derives from the C++ MIRO_EXPORT_TYPES / MIRO_EXPORT_COMMANDS
// registrations; nothing here is hand-maintained against the C++ side.

import { makeBackend } from './generated/schema.backend';
import { makeBridge } from './generated/schema.bridge';
import { localTransport } from './transport';

const api = makeBridge(localTransport, makeBackend);

async function main(): Promise<void>
{
    await api.ping();

    const list = await api.listUsers();
    console.log('[main] users:', list.users);

    const user = await api.getUser({ id: 'u1' });
    console.log('[main] user:', user);

    const sent = await api.sendMessage({
        fromUserId: 'u1',
        text: 'Hello from Node!',
    });
    console.log('[main] sent:', sent.message);

    await api.logEvent({
        severity: 'Warning',
        message: 'something to investigate',
    });
}

main().catch((err) =>
{
    console.error(err);
    process.exit(1);
});
