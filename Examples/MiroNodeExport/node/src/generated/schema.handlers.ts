import type * as T from './schema';

export type Handlers = {
    getUser(req: T.GetUserRequest): T.User | Promise<T.User>;
    listUsers(): T.ListUsersResponse | Promise<T.ListUsersResponse>;
    sendMessage(req: T.SendMessageRequest): T.SendMessageResponse | Promise<T.SendMessageResponse>;
    logEvent(req: T.LogRequest): void | Promise<void>;
    ping(): void | Promise<void>;
};

export class UnknownCommandError extends Error
{
    httpStatus = 404;
    constructor(command: string)
    {
        super(`Unknown command: ${command}`);
    }
}

export async function dispatch(handlers: Handlers, command: string, payload: unknown): Promise<unknown>
{
    switch (command)
    {
        case 'getUser': return await handlers.getUser(payload as T.GetUserRequest);
        case 'listUsers': return await handlers.listUsers();
        case 'sendMessage': return await handlers.sendMessage(payload as T.SendMessageRequest);
        case 'logEvent': return await handlers.logEvent(payload as T.LogRequest);
        case 'ping': return await handlers.ping();
        default: throw new UnknownCommandError(command);
    }
}
