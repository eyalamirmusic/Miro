import type * as T from './schema';

export type Invoke = (command: string, payload: unknown) => Promise<unknown>;

export function makeBackend(invoke: Invoke)
{
    return {
        getUser: (req: T.GetUserRequest): Promise<T.User> =>
            invoke('getUser', req) as Promise<T.User>,
        listUsers: (): Promise<T.ListUsersResponse> =>
            invoke('listUsers', {}) as Promise<T.ListUsersResponse>,
        sendMessage: (req: T.SendMessageRequest): Promise<T.SendMessageResponse> =>
            invoke('sendMessage', req) as Promise<T.SendMessageResponse>,
        logEvent: (req: T.LogRequest): Promise<void> =>
            invoke('logEvent', req) as Promise<void>,
        ping: (): Promise<void> =>
            invoke('ping', {}) as Promise<void>,
    };
}
