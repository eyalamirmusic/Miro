// Node-side implementations for the commands declared on the C++ side.
//
// The generated Handlers type (schema.handlers.ts) gives us a typed
// contract — TypeScript fails to compile if a handler's signature
// drifts from the C++ MIRO_EXPORT_COMMANDS registration. Each
// implementation here is a console.log stub returning a placeholder
// response, but in a real app this is where the actual logic lives
// (database access, business rules, etc.).

import type * as T from './generated/schema';
import type { Handlers } from './generated/schema.handlers';

export const handlers: Handlers = {
    getUser(req: T.GetUserRequest): T.User
    {
        console.log('[handlers] getUser', req);
        return { id: req.id, name: 'Stub User', isAdmin: false };
    },

    listUsers(): T.ListUsersResponse
    {
        console.log('[handlers] listUsers');
        return {
            users: [
                { id: 'u1', name: 'Alice', isAdmin: true },
                { id: 'u2', name: 'Bob', isAdmin: false },
            ],
        };
    },

    sendMessage(req: T.SendMessageRequest): T.SendMessageResponse
    {
        console.log('[handlers] sendMessage', req);
        return {
            message: {
                id: 'm-' + Math.random().toString(36).slice(2, 8),
                fromUserId: req.fromUserId,
                text: req.text,
                timestamp: Date.now(),
            },
        };
    },

    logEvent(req: T.LogRequest): void
    {
        console.log(`[handlers] logEvent [${req.severity}] ${req.message}`);
    },

    ping(): void
    {
        console.log('[handlers] ping');
    },
};
